# Prompt para a IA do Canva — infográfico 60 × 15 cm

Uso: no Canva Pro, crie um design em **Custom size → 60 × 15 cm**, suba `foto-bancada.jpg`, `infografico-uerj-sem-muros-60x15.png` (referência) e, no Brand Kit, as cores e as fontes de `fontes/` (Source Sans 3). Cole o prompt abaixo na IA e substitua os textos gerados pelos textos exatos do prompt. QR: Apps → QR Code → `github.com/italorosq/Girino`. Exporte PDF Print para a gráfica.

Atenção: a exportação PNG do Canva pode sair abaixo de 300 dpi; para o SDA, use o PNG de `docs/images/` (mesmo layout, 7087 × 1772 px).

---

Crie um infográfico horizontal (banner) de 60 × 15 cm, em português do Brasil, sobre um projeto acadêmico de engenharia, para a 23ª Semana de Graduação / 34ª UERJ Sem Muros. Público: banca e visitantes. Estilo institucional, técnico e sóbrio — nada infantil, nada de clip-art, sem sombras fortes nem excesso de gradientes. Fundo geral #E4EBF3; painéis brancos com borda fina #D9E3EF e cantos arredondados.

TEMA: modernização das plataformas robóticas do LaRA (Laboratório de Robótica e Automação — UERJ / CETREINA / EIC). O Girino é o PRIMEIRO MÓDULO ENTREGUE dessa modernização — não é um projeto separado.

Título (usar exatamente): "Modernização de Plataformas Robóticas e Desenvolvimento de Infraestrutura para Experimentação em Algoritmos de Robótica".

LAYOUT — 6 painéis lado a lado, com pequenos espaços entre eles (~0,1 cm) e estas larguras relativas:
1. Identidade — 15,6% (fundo azul-escuro #0B2C57, texto branco)
2. Problema — 12,7% (barra vermelha #C0392B)
3. Tese e Objetivo — 15,6% (barra azul #1660A8)
4. Método — 22,0% (barra azul #1660A8)
5. Resultados — 19,1% (barra verde-água #0E9594)
6. Impacto — 14,2% (barra verde-água #0E9594)
Cada painel tem uma barra vertical colorida de ~14 px ao lado do título da seção. Toques sutis de vida: sombra leve nos painéis, faixa de fundo tingida no cabeçalho de cada painel na cor da seção (~6% de opacidade), ícones dentro de badges circulares claros, chips com preenchimento tingido, e nos cartões de números um numeral gigante "fantasma" ao fundo (~7% de opacidade). Ícones lineares, traço fino, sem preenchimento.

PAINEL 1 — IDENTIDADE (azul-escuro #0B2C57, texto branco):
- Etiqueta no topo, em âmbar #F2A100: EIC · CETREINA/UERJ · LaRA
- Título acima, grande (ocupa cerca de 6 linhas)
- Subtítulo: Modernizar e manter o que a instituição já tem: o Girino como primeiro produto e o caminho documentado para quem vier depois.
- Quatro chips: Modernizar o que existe · Girino: primeiro produto · Plataforma aberta · Formação do bolsista
- Na base: um QR code branco com a legenda "Código aberto" (apontando para github.com/italorosq/Girino) e um mascote discreto de girino (tadpole) em traço fino branco, bem apagado, como marca d'água.

PAINEL 2 — PROBLEMA (vermelho #C0392B), 4 itens com ícones lineares:
1. Equipamento preservado e fora de uso — A estrutura continua íntegra, mas o sistema de controle envelhece, o suporte do fabricante acaba e as peças saem de linha. (ícone: base móvel com braço)
2. Camada de controle fechada — O que existe não admite modificação, expansão nem integração com as ferramentas atuais. (ícone: cadeado sobre conector)
3. Tentativas anteriores abandonadas — A curva de aprendizado inicial não se sustentou e o equipamento voltou para a prateleira. (ícone: linha do tempo interrompida por um X)
4. Formação sem prática — Controle, cinemática e processamento de sinais estudados sem contato com hardware real, ruído e limitações. (ícone: livro aberto com circuito)

PAINEL 3 — TESE E OBJETIVO (azul #1660A8):
- Duas premissas em destaque (com aspas decorativas grandes ao fundo, bem apagadas):
  "Manter o equipamento em uso vale tanto quanto adquiri-lo."
  "Baixar a barreira de entrada é o que devolve o equipamento ao uso."
- OBJETIVO: Modernizar e manter as plataformas do laboratório, com controladores abertos de baixo custo, e transformar o processo em formação técnica para o bolsista e em recurso aberto para a comunidade acadêmica. O Girino é o primeiro produto. (destacar a última frase)
- Nota de rodapé, em itálico: Bolsista recém-iniciado e sem verba de material: o que já roda usa os componentes que o laboratório tem em estoque.

PAINEL 4 — MÉTODO (azul #1660A8), 4 etapas numeradas em círculo:
1. Caracterizar o que existe — Entender a estrutura, medir o que ainda funciona e registrar o que precisa ser trocado.
2. Começar pelo Girino — Um motor, um driver e um encoder, com interface no navegador, para validar o caminho em escala pequena.
3. Avançar por etapas — Cada função nova é validada em bancada antes de entrar no equipamento, e o que dá errado é registrado junto com a solução.
4. Documentar enquanto acontece — Esquemas, medições, listas de materiais e código aberto, para que a próxima pessoa não recomece do zero.
- Dois chips: "Ciclos: bancada → plataforma montada → uso real" e "Adaptações mecânicas impressas em 3D no próprio laboratório"

PAINEL 5 — RESULTADOS (verde-água #0E9594), 4 métricas grandes em grade 2 × 2 (número gigante + rótulo):
- 2 — malhas PID em bancada: velocidade e posição, com auto-sintonia embarcada
- 3 — métodos de sintonia comparados: Ziegler-Nichols, Tyreus-Luyben e Cohen-Coon
- 600 — pulsos por revolução no encoder incremental lido por interrupção
- 0 — programas a instalar: o controle é feito pelo navegador, sem internet
- Abaixo das métricas, uma FOTO REAL da bancada (anexada): motor DC com encoder e roda, driver e caixa impressa em 3D sobre base de acrílico. NÃO substituir por banco de imagens. Legenda sobre a foto: "Bancada de validação do Girino — ESP8266 + driver MOSFET + encoder LPD3806-600BM"
- Faixa escura no pé do painel: Sem verba de material: componentes do estoque do laboratório · PID e auto-sintonia validados em bancada · placa de circuito impresso em projeto no KiCad e caixa em 3D nas próximas etapas

PAINEL 6 — IMPACTO (verde-água #0E9594), 4 itens numerados:
1. Equipamento de volta ao uso — Em vez de comprar de novo, manter e modernizar o que a instituição já pagou.
2. Entrada barata para a robótica — O Girino serve a disciplinas, extensão e trabalhos de conclusão, e a bancada é replicável em outros laboratórios.
3. Formação técnica — O bolsista percorre o ciclo completo de um projeto de sistemas embarcados, da medição de bancada à documentação de entrega.
4. Método replicável — Caracterizar o que existe, adotar arquitetura aberta e documentar em processo contínuo, o que serve a outros equipamentos da instituição.
- Faixa âmbar no pé: Diversidade que inspira, conhecimento que transforma

HIERARQUIA DE TEXTO (para 60 × 15 cm, legível a ~1,5 m):
- Título do trabalho: ~19–20 pt (≈ 7,5 mm de altura)
- Títulos dos painéis: ~14 pt (≈ 5 mm)
- Corpo: 11–12 pt (≈ 3,8–4,2 mm) — nunca abaixo de ~10 pt
- Números das métricas: ~36 pt (≈ 12,7 mm)
- Chips: ~10 pt

TIPOGRAFIA E CORES:
- Fonte sem serifa Source Sans 3 (ou Source Sans Pro)
- Paleta: #0B2C57 (azul-escuro), #1660A8 (azul), #0E9594 (verde-água), #F2A100 (âmbar), #C0392B (vermelho), #D9E3EF (borda), #E4EBF3 (fundo)

FATOS QUE PODEM SER AFIRMADOS (não inventar além disso): firmware em modo hardware real (sem simulador); PID de velocidade e de posição em malha fechada com auto-sintonia embarcada, validados em bancada; 3 métodos de sintonia comparados; encoder de 600 pulsos por revolução lido por interrupção; controle pelo navegador, sem instalar nada e sem internet.

NÃO INCLUIR: nomes de autores, orientador, curso ou unidade acadêmica (ficam no cabeçalho do pôster, fora do infográfico); licenças; referências bibliográficas; metragem do laboratório; modelo, dimensões ou graus de liberdade dos equipamentos a modernizar; qualquer número, resultado ou prêmio que não esteja neste prompt.

---

## Refinamentos (se a IA do Canva fugir do alvo)

- "Mantenha este layout, mas troque toda a tipografia por Source Sans Pro e reduza as sombras."
- "O painel Resultados deve ficar exatamente 2 × 2 com 4 cartões iguais; aumente os números em 20%."
- "Deixe os ícones lineares, traço fino, dentro de círculos claros, sem preenchimento."
- "Aumente todo o corpo de texto em ~10% — precisa ser legível a 1,5 m no banner."
