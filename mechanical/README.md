# Peças Mecânicas — Girino

Modelos 3D para impressão da caixa/estrutura da plataforma Girino.

## Estrutura

```
mechanical/
├── cad/      # Arquivos-fonte FreeCAD (.FCStd) — editáveis
├── stl/      # Arquivos STL — prontos para impressão 3D
└── step/     # Arquivos STEP — para referência e montagem virtual
```

## Peças

| Peça | Arquivo | Descrição |
|---|---|---|
| Caixa superior | `girino-enclosure-top.stl` | Tampa da caixa protetora |
| Caixa inferior | `girino-enclosure-bottom.stl` | Base com fixações para componentes |
| Suporte encoder | `girino-encoder-mount.stl` | Suporte para fixação do encoder |

## Configurações de Impressão Recomendadas

| Parâmetro | Valor |
|---|---|
| Material | PLA ou PETG |
| Preenchimento | 20% (mínimo) |
| Perímetros | 3 |
| Altura de camada | 0.2 mm |
| Suportes | Conforme necessário |

## Editando os Modelos

1. Instale o [FreeCAD](https://www.freecad.org/)
2. Abra os arquivos `.FCStd` em `cad/`
3. Faça suas alterações
4. Exporte STL em **File → Export**
5. Salve o arquivo-fonte atualizado em `cad/`

## Convenções

- Nomes de arquivo descritivos em inglês
- STLs devem ser testados em slicer antes de commitar
- Manter arquivos STEP atualizados para referência
