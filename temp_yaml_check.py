import yaml
p = '.github/workflows/deploy.yml'
with open(p, encoding='utf-8') as f:
    t = f.read()
print('line1', repr(t.splitlines()[0]))
try:
    o = yaml.safe_load(t)
    print('ok', list(o.keys()) if isinstance(o, dict) else type(o))
except Exception as e:
    import traceback
    traceback.print_exc()
