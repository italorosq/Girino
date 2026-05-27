# Atualização OTA — Girino

O Girino suporta atualização de firmware via Wi-Fi (Over-The-Air), eliminando a necessidade de cabo USB após a primeira gravação.

## Como Funciona

Usamos a biblioteca [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA), que fornece uma interface web para upload do firmware.

## Primeira Gravação (via USB)

A primeira gravação deve ser feita com cabo USB:

```bash
cd firmware
pio run -t upload
```

Após a gravação, o ESP8266 conecta ao Wi-Fi e exibe seu IP no monitor serial.

## Métodos de Atualização OTA

### 1. Interface Web (Recomendado para alunos)

1. Acesse `http://<IP_DO_ESP8266>/update` no navegador
2. Selecione o arquivo de firmware `.bin`
3. Clique em **Upload**
4. Aguarde a reinicialização automática

### 2. PlatformIO (Desenvolvedores)

```bash
# Via mDNS
pio run -t upload --upload-port girino.local

# Via IP
pio run -t upload --upload-port 192.168.1.100
```

### 3. Linha de comando (curl)

```bash
# Gerar o firmware .bin
pio run

# Upload via curl
curl -F "firmware=@.pio/build/girino/firmware.bin" \
     http://192.168.1.100/update \
     -u admin:sua_senha_ota
```

## Segurança

- A senha OTA é configurada no `.env` (`OTA_PASSWORD`)
- Em ambiente de laboratório, use uma senha simples para facilitar
- Para produção, use senha forte e considere HTTPS

## Resolução de Problemas

| Problema | Solução |
|---|---|
| OTA não funciona | Verificar se WiFi está conectado |
| Upload falha | Verificar espaço em flash disponível |
| Device não encontrado | Verificar IP no monitor serial |
| Senha incorreta | Re-gravar via USB com nova senha |

## Verificação

Após atualização OTA bem-sucedida:
1. O LED onboard pisca brevemente
2. O ESP8266 reinicia automaticamente
3. Verificar a versão em `http://<IP>/api/status`
