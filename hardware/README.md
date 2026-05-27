# Hardware — Girino

Design de PCB para a plataforma didática Girino, desenvolvido em KiCad.

## Estrutura

```
hardware/
├── kicad/          # Projeto KiCad (arquivos-fonte editáveis)
├── gerbers/        # Arquivos Gerber empacotados (.zip) para fabricação
├── bom/            # Bill of Materials (lista de materiais) em CSV
└── schematics/     # PDFs exportados do esquemático
```

## Componentes Principais

| Componente | Qtd | Descrição |
|---|---|---|
| ESP8266 WROOM | 1 | Microcontrolador Wi-Fi |
| LPD3806-600BM | 1 | Encoder incremental 600 PPR |
| Driver MOSFET | 1 | Módulo driver para motor DC |
| Regulador de tensão | 1 | Conversão de tensão para alimentação do circuito |

## Fabricação da PCB

### Gerando Gerbers no KiCad

1. Abra o projeto em `kicad/`
2. Vá em **File → Plot**
3. Selecione as camadas necessárias
4. Gere os arquivos drill
5. Empacote tudo em um `.zip` e salve em `gerbers/`

### Enviando para Fabricação

Arquivos `.zip` em `gerbers/` prontos para enviar a fabricantes como:
- [JLCPCB](https://jlcpcb.com/)
- [PCBWay](https://www.pcbway.com/)
- [Eletrrow](https://www.electrow.com/)

## Convenções

- Nomes de arquivo incluem versão: `gerbers-v1.0.zip`, `bom-v1.0.csv`
- Incluir URL do repositório no silkscreen da PCB
- Arquivos temporários de backup (`.bak`, `*-backups/`) são ignorados pelo git
