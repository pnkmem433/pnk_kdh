import * as dotenv from 'dotenv';
dotenv.config();

import { NestFactory } from '@nestjs/core';
import { SwaggerModule, DocumentBuilder } from '@nestjs/swagger';
import { ExpressAdapter } from '@nestjs/platform-express';
import * as express from 'express';
import * as bodyParser from 'body-parser';

import { AppModule } from './app.module';
import { buildOtaLogContext, otaLog, parseFirmwareDownloadInput } from './modules/firmwareDownload/ota-log.util';

async function bootstrap() {
  const server = express();

  server.use(
    '/firmwareDownload',
    bodyParser.text({ type: ['application/json', 'text/plain'] }),
  );
  server.use('/firmwareDownload', (req, _res, next) => {
    if (req.method === 'POST') {
      const payload = parseFirmwareDownloadInput(typeof req.body === 'string' ? req.body : '');
      const context = buildOtaLogContext(payload, req.headers as Record<string, string | string[] | undefined>);
      otaLog(context, `요청 수신: ${req.method} ${req.originalUrl}`);
    } else {
      otaLog({}, `요청 수신: ${req.method} ${req.originalUrl}`);
    }
    next();
  });

  const app = await NestFactory.create(AppModule, new ExpressAdapter(server));

  app.enableCors({
    origin: ['http://gym907-0001.iptime.org', 'http://localhost:3000', 'http://localhost:3004'],
    methods: 'GET,HEAD,PUT,PATCH,POST,DELETE,OPTIONS',
    credentials: true,
  });

  const options = new DocumentBuilder()
    .setTitle('SmartPlug OTA API')
    .setDescription('SmartPlug OTA dashboard API')
    .setVersion('1.0')
    .addBearerAuth()
    .build();

  const document = SwaggerModule.createDocument(app, options);
  SwaggerModule.setup('api', app, document);

  const port = parseInt(process.env.PORT ?? '3004', 10);
  await app.listen(port, '0.0.0.0');
  console.log(`[서버] OTA 대시보드 실행 중: http://127.0.0.1:${port}`);
}

bootstrap();
