# Contribuindo para o Girino

Obrigado pelo interesse em contribuir com o projeto Girino!

## Como Contribuir

### Reportando Problemas

1. Verifique se o problema já não foi reportado nas [Issues](../../issues)
2. Abra uma nova issue com:
   - Descrição clara do problema
   - Passos para reproduzir
   - Comportamento esperado vs. observado
   - Fotos/screenshots se aplicável
   - Versão do firmware/hardware

### Enviando Contribuições

1. **Fork** o repositório
2. Crie uma **branch** descritiva:
   ```bash
   git checkout -b feature/nome-da-feature
   # ou
   git checkout -b fix/nome-do-fix
   ```
3. Faça suas alterações seguindo as convenções abaixo
4. **Commit** com mensagens claras (em português ou inglês)
5. Abra um **Pull Request** descrevendo as mudanças

### Convenções

#### Firmware
- Seguir padrão PlatformIO (`src/`, `include/`, `lib/`)
- Um arquivo `.h`/`.cpp` por módulo funcional
- Definições de pinos centralizadas em `pins.h`
- Constantes em `config.h`
- Nunca commitar credenciais (use `.env`)

#### Hardware
- Projetos KiCad na pasta `hardware/kicad/`
- Gerbers empacotados em `.zip` com versão em `hardware/gerbers/`
- BOM em formato CSV em `hardware/bom/`
- Incluir URL do repositório no silkscreen da PCB

#### Mecânico
- Arquivos-fonte FreeCAD (`.FCStd`) em `mechanical/cad/`
- STLs exportados em `mechanical/stl/`
- STEP para referência em `mechanical/step/`
- Documentar configurações de impressão no README

#### Documentação
- Markdown (`.md`) para todos os textos
- Imagens em `docs/images/`
- Manter os READMEs de cada pasta atualizados

### Organização de Pastas

```
firmware/     → Código fonte do firmware (PlatformIO)
hardware/     → PCBs, esquemáticos, gerbers, BOM
mechanical/   → Peças 3D (CAD, STL, STEP)
docs/         → Documentação técnica
examples/     → Exemplos didáticos
gallery/      → Fotos e renders
```

### Revisão de Código

Todos os PRs serão revisados considerando:
- Funcionalidade e correção
- Adesão às convenções do projeto
- Impacto na experiência didática
- Compatibilidade com o hardware existente

## Dúvidas?

Abra uma [Issue](../../issues) com a label `question`.
