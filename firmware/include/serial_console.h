/**
 * serial_console.h — Console de comandos via serial (USB)
 *
 * Permite controlar o Girino sem Wi-Fi, digitando comandos no monitor
 * serial (115200 baud). Útil para bancada, testes rápidos e para
 * roteirizar experimentos. Digite 'help' para ver os comandos.
 */

#ifndef SERIAL_CONSOLE_H
#define SERIAL_CONSOLE_H

/**
 * Inicializa o console (imprime a dica de ajuda).
 */
void serialConsoleInit();

/**
 * Deve ser chamada no loop(): lê comandos da serial e executa.
 */
void serialConsoleUpdate();

#endif // SERIAL_CONSOLE_H
