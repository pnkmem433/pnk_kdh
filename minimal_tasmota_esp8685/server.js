const http = require('http');
const fs = require('fs');
const path = require('path');
const { URL } = require('url');
const mqtt = require('mqtt');

const HOST = process.env.HOST || '0.0.0.0';
const PORT = Number(process.env.PORT || 8787);
const MQTT_HOST = process.env.MQTT_HOST || 'api.pnkslab.com';
const MQTT_PORT = Number(process.env.MQTT_PORT || 1884);
const MQTT_USER = process.env.MQTT_USER || 'pnks';
const MQTT_PASS = process.env.MQTT_PASS || 'pnks1111';
const TOPIC_PREFIX = process.env.TOPIC_PREFIX || 'smart_plug';
const TELE_PREFIX = process.env.TELE_PREFIX || 'tele';
const STATUS_EXPECTED_INTERVAL_MS = Number(process.env.STATUS_EXPECTED_INTERVAL_MS || 1000);
const OFFLINE_MISSED_MESSAGES = Number(process.env.OFFLINE_MISSED_MESSAGES || 3);
const OFFLINE_TIMEOUT_MS = Number(
  process.env.OFFLINE_TIMEOUT_MS || STATUS_EXPECTED_INTERVAL_MS * OFFLINE_MISSED_MESSAGES
);

const dashboardDir = __dirname;
const indexPath = path.join(dashboardDir, 'index.html');

const plugs = new Map();
const clients = new Set();
let mqttConnected = false;

function nowIso() {
  return new Date().toISOString();
}

function normalizeState(value) {
  const state = String(value || '').toLowerCase();
  if (state === 'on') return 'on';
  if (state === 'off') return 'off';
  return 'unknown';
}

function shortIdFromUuid(uuid) {
  const text = String(uuid || '').toUpperCase();
  return text.length > 6 ? text.slice(-6) : text;
}

function webserverModeFromText(value) {
  const text = String(value || '').toLowerCase();
  if (text === 'admin') return 2;
  if (text === 'user') return 1;
  if (text === 'off') return 0;
  return null;
}

function ensurePlug(uuid) {
  const fullUuid = String(uuid || '').toUpperCase();
  if (!plugs.has(fullUuid)) {
    plugs.set(fullUuid, {
      uuid: fullUuid,
      shortId: shortIdFromUuid(fullUuid),
      state: 'unknown',
      webserver: null,
      ssid: '',
      ipAddress: '',
      lastSeen: nowIso(),
      online: true,
      metrics: {}
    });
  }
  return plugs.get(fullUuid);
}

function ensurePlugByUuid(uuid) {
  const fullUuid = String(uuid || '').toUpperCase();
  const shortId = shortIdFromUuid(fullUuid);

  if (!plugs.has(fullUuid) && plugs.has(shortId)) {
    const tempPlug = plugs.get(shortId);
    plugs.delete(shortId);
    tempPlug.uuid = fullUuid;
    tempPlug.shortId = shortId;
    plugs.set(fullUuid, tempPlug);
  }

  return ensurePlug(fullUuid);
}

function ensurePlugByShortId(shortId) {
  const target = String(shortId || '').toUpperCase();
  for (const plug of plugs.values()) {
    if (plug.shortId === target || plug.uuid === target || plug.uuid.endsWith(target)) {
      return plug;
    }
  }

  const tempPlug = ensurePlug(target);
  tempPlug.shortId = target;
  return tempPlug;
}

function hydrateOnlineState(plug) {
  plug.online = (Date.now() - new Date(plug.lastSeen).getTime()) <= OFFLINE_TIMEOUT_MS;
  return plug;
}

function sendSse(res, event, data) {
  res.write(`event: ${event}\n`);
  res.write(`data: ${JSON.stringify(data)}\n\n`);
}

function broadcast(event, data) {
  for (const client of clients) {
    sendSse(client, event, data);
  }
}

function buildSnapshot() {
  const list = [...plugs.values()]
    .map((plug) => hydrateOnlineState(plug))
    .sort((a, b) => a.uuid.localeCompare(b.uuid));

  return {
    broker: {
      host: MQTT_HOST,
      port: MQTT_PORT
    },
    mqtt: {
      connected: mqttConnected
    },
    topicPrefix: TOPIC_PREFIX,
    plugs: list
  };
}

function sendJson(res, statusCode, data) {
  const body = JSON.stringify(data);
  res.writeHead(statusCode, {
    'Content-Type': 'application/json; charset=utf-8',
    'Content-Length': Buffer.byteLength(body),
    'Cache-Control': 'no-cache'
  });
  res.end(body);
}

function serveIndex(res) {
  const body = fs.readFileSync(indexPath, 'utf8');
  res.writeHead(200, {
    'Content-Type': 'text/html; charset=utf-8',
    'Content-Length': Buffer.byteLength(body)
  });
  res.end(body);
}

function parseBody(req) {
  return new Promise((resolve, reject) => {
    const chunks = [];
    req.on('data', (chunk) => chunks.push(chunk));
    req.on('end', () => {
      const text = Buffer.concat(chunks).toString('utf8');
      if (!text) {
        resolve({});
        return;
      }
      try {
        resolve(JSON.parse(text));
      } catch (error) {
        reject(error);
      }
    });
    req.on('error', reject);
  });
}

function mergeEnergyMetrics(target, energy) {
  if (!energy) return;
  target.metrics.total = energy.Total;
  target.metrics.yesterday = energy.Yesterday;
  target.metrics.daily = energy.Today;
  target.metrics.power = energy.Power;
  target.metrics.apparentPower = energy.ApparentPower;
  target.metrics.reactivePower = energy.ReactivePower;
  target.metrics.factor = energy.Factor;
  target.metrics.voltage = energy.Voltage;
  target.metrics.current = energy.Current;
  target.metrics.energy_available = true;
}

function updatePlugSeen(plug) {
  plug.lastSeen = nowIso();
  plug.online = true;
}

const mqttClient = mqtt.connect(`mqtt://${MQTT_HOST}:${MQTT_PORT}`, {
  username: MQTT_USER,
  password: MQTT_PASS,
  reconnectPeriod: 3000,
  connectTimeout: 10000,
  clean: true
});

mqttClient.on('connect', () => {
  mqttConnected = true;
  mqttClient.subscribe(`${TOPIC_PREFIX}/+/status`);
  mqttClient.subscribe(`${TOPIC_PREFIX}/+/metrics`);
  mqttClient.subscribe(`${TELE_PREFIX}/+/STATE`);
  mqttClient.subscribe(`${TELE_PREFIX}/+/SENSOR`);
  mqttClient.subscribe(`${TELE_PREFIX}/+/INFO2`);
  mqttClient.subscribe(`${TELE_PREFIX}/+/LWT`);
  broadcast('mqtt', { connected: true });
  broadcast('snapshot', buildSnapshot());
  console.log(`[MQTT] connected to ${MQTT_HOST}:${MQTT_PORT}`);
});

mqttClient.on('reconnect', () => {
  mqttConnected = false;
  broadcast('mqtt', { connected: false });
  console.log('[MQTT] reconnecting');
});

mqttClient.on('close', () => {
  mqttConnected = false;
  broadcast('mqtt', { connected: false });
  console.log('[MQTT] connection closed');
});

mqttClient.on('error', (error) => {
  mqttConnected = false;
  broadcast('mqtt', { connected: false });
  console.error('[MQTT] error', error.message);
});

mqttClient.on('message', (topic, payloadBuffer) => {
  const payloadText = payloadBuffer.toString('utf8');
  const parts = topic.split('/');
  if (parts.length < 3) return;

  const [prefix, id, leaf] = parts;

  try {
    if (prefix === TOPIC_PREFIX) {
      const payload = JSON.parse(payloadText);
      const plug = ensurePlugByUuid(id);
      updatePlugSeen(plug);

      if (leaf === 'status') {
        if (payload.state !== undefined) {
          plug.state = normalizeState(payload.state);
        }
        if (typeof payload.webserver === 'number') {
          plug.webserver = payload.webserver;
        }
      }

      if (leaf === 'metrics') {
        plug.metrics = { ...plug.metrics, ...payload };
        if (payload.state !== undefined) {
          plug.state = normalizeState(payload.state);
        }
        if (typeof payload.webserver === 'number') {
          plug.webserver = payload.webserver;
        }
      }

      broadcast('plug', hydrateOnlineState(plug));
      return;
    }

    if (prefix === TELE_PREFIX && id.startsWith('tasmota_')) {
      const shortId = id.replace(/^tasmota_/i, '').toUpperCase();
      const plug = ensurePlugByShortId(shortId);

      if (leaf === 'LWT') {
        plug.online = String(payloadText).trim().toLowerCase() === 'online';
        if (plug.online) {
          plug.lastSeen = nowIso();
        }
        broadcast('plug', hydrateOnlineState(plug));
        return;
      }

      const payload = JSON.parse(payloadText);
      updatePlugSeen(plug);

      if (leaf === 'STATE') {
        if (payload.POWER !== undefined) {
          plug.state = normalizeState(payload.POWER);
        }
        if (payload.Wifi && payload.Wifi.SSId) {
          plug.ssid = payload.Wifi.SSId;
        }
        if (payload.IPAddress) {
          plug.ipAddress = payload.IPAddress;
        }
      }

      if (leaf === 'SENSOR' && payload.ENERGY) {
        mergeEnergyMetrics(plug, payload.ENERGY);
      }

      if (leaf === 'INFO2' && payload.Info2) {
        if (payload.Info2.WebServerMode) {
          plug.webserver = webserverModeFromText(payload.Info2.WebServerMode);
        }
        if (payload.Info2.IPAddress) {
          plug.ipAddress = payload.Info2.IPAddress;
        }
      }

      broadcast('plug', hydrateOnlineState(plug));
    }
  } catch (error) {
    console.error('[MQTT] parse error', topic, error.message);
  }
});

setInterval(() => {
  let changed = false;
  for (const plug of plugs.values()) {
    const before = plug.online;
    hydrateOnlineState(plug);
    if (before !== plug.online) {
      changed = true;
      broadcast('plug', plug);
    }
  }
  if (changed) {
    broadcast('snapshot', buildSnapshot());
  }
}, 5000);

const server = http.createServer(async (req, res) => {
  const url = new URL(req.url, `http://${req.headers.host}`);

  if (req.method === 'GET' && url.pathname === '/') {
    serveIndex(res);
    return;
  }

  if (req.method === 'GET' && url.pathname === '/api/plugs') {
    sendJson(res, 200, buildSnapshot());
    return;
  }

  if (req.method === 'GET' && url.pathname === '/api/events') {
    res.writeHead(200, {
      'Content-Type': 'text/event-stream; charset=utf-8',
      'Cache-Control': 'no-cache, no-transform',
      Connection: 'keep-alive',
      'Access-Control-Allow-Origin': '*'
    });
    res.write('\n');
    clients.add(res);
    sendSse(res, 'snapshot', buildSnapshot());

    const heartbeat = setInterval(() => {
      res.write(': heartbeat\n\n');
    }, 15000);

    req.on('close', () => {
      clearInterval(heartbeat);
      clients.delete(res);
    });
    return;
  }

  if (req.method === 'POST' && /^\/api\/plugs\/[^/]+\/command$/.test(url.pathname)) {
    const parts = url.pathname.split('/');
    const uuid = decodeURIComponent(parts[3]);

    try {
      const body = await parseBody(req);
      const cmd = String(body.cmd || '').toLowerCase();
      if (!['on', 'off', 'status'].includes(cmd)) {
        sendJson(res, 400, { error: '지원하지 않는 명령입니다.' });
        return;
      }

      const topic = `${TOPIC_PREFIX}/${uuid}/command`;
      const payload = JSON.stringify({ cmd });
      mqttClient.publish(topic, payload, { qos: 0, retain: false }, (error) => {
        if (error) {
          sendJson(res, 500, { error: error.message });
          return;
        }
        sendJson(res, 200, { ok: true, topic, payload });
      });
    } catch (error) {
      sendJson(res, 400, { error: 'JSON body parsing failed' });
    }
    return;
  }

  sendJson(res, 404, { error: 'not found' });
});

server.listen(PORT, HOST, () => {
  console.log(`[HTTP] dashboard server listening on http://${HOST}:${PORT}`);
});
