# Girino — Carregador de credenciais (.env)
#
# O PlatformIO não interpola variáveis de um arquivo .env nas build_flags.
# Este script (extra_script) lê o .env da raiz do projeto, remove aspas
# externas e injeta as credenciais como -D no compilador.
#
# Prioridade: variável de ambiente do sistema > arquivo .env

Import("env")

import os

dotenv_path = os.path.join(env.Dir("#").abspath, ".env")

REQUIRED_VARS = ("WIFI_SSID", "WIFI_PASSWORD", "OTA_PASSWORD")


def parse_dotenv(path):
    values = {}
    if not os.path.isfile(path):
        return values
    with open(path, "r", encoding="utf-8-sig") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, _, value = line.partition("=")
            value = value.strip()
            if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
                value = value[1:-1]
            values[key.strip()] = value
    return values


dotenv = parse_dotenv(dotenv_path)

missing = []
for name in REQUIRED_VARS:
    value = os.environ.get(name) or dotenv.get(name, "")
    if value:
        env.Append(BUILD_FLAGS=['-D%s=\\"%s\\"' % (name, value)])
    else:
        missing.append(name)

if missing:
    print("[load_env] AVISO: credenciais ausentes: %s" % ", ".join(missing))
    print("[load_env] Copie .env.example para .env e preencha as credenciais.")
