# Girino: plataforma didática de baixo custo para ensino de controle de motores DC via navegador

**Autor:** Ítalo Rosa Gonçalves
**Orientador:** Prof. Ângelo Mondaini Calvão
**Laboratório:** LaRA — Laboratório de Robótica e Automação (UERJ)
**Modalidade:** Estágio Interno Complementar
**ODS:** 4 — Educação de Qualidade

## Resumo

O ensino de controle exige prática experimental, mas bancadas comerciais são caras e as alternativas de baixo custo dependem de software proprietário ou instalações locais, restringindo o acesso dos estudantes. Alinhado ao ODS 4 (Educação de Qualidade), este trabalho desenvolve o Girino, plataforma didática de baixo custo para controle de motores DC, acessível apenas pelo navegador.

A bancada reúne microcontrolador com Wi-Fi, encoder de realimentação e driver de potência. O firmware implementa controle de velocidade e de posição em malha fechada, com auto-sintonia e comparação de métodos de sintonia. A interface web embarcada permite operar o motor e acompanhar a resposta em qualquer dispositivo, sem instalar nada, e o firmware é atualizado por OTA. O hardware também foi projetado no laboratório.

A bancada foi montada e validada: os controles de velocidade e posição operam em malha fechada, a auto-sintonia identifica o sistema e sugere ganhos, e a interface funciona sem internet. A caracterização do motor e a calibração da fiação também foram tratadas.

O Girino se mostra uma plataforma funcional e acessível, que dispensa MATLAB, Python ou qualquer instalação e facilita a manutenção das bancadas via OTA. Como continuidade, prevê-se a modelagem do sistema, a comparação experimental dos métodos e a avaliação pedagógica com alunos.
