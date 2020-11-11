/**
 * @file Utils.h
 * @author Kevin de la Coba Malam (kevin.coba@estudiante.uam.es)
 *         Jose Manuel Freire     (jose.freire@estudiante.uam.es)
 * @brief Archivo donde se definen los prototipos de las funciones en Utils.c
 * 
 * Modulo: Utils
 * 
 * @version 1
 * @date 2020-04-28
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef UTILS_H
#define UTILS_H

/**
 * @brief Calcula el logaritmo entero en base 2
 * 
 * @param n Numero a calcular
 * @return int Resultado de la operación
 */
int compute_log(int n);

/**
 * @brief Limpia la pantalla
 * 
 * @return Status Ejecución de la función
 */
Status clear_screen();

/**
 * @brief Imprime un vector por pantalla en modo texto
 * 
 * @param vector Vector a imprimir
 * @param len Longitud del vector
 * @return Status Ejecución de la función
 */
Status print_vector(int *vector, int len);

/**
 * @brief Pinta un vector por pantalla. Si es lo suficientemente pequeño
 * utilizará una representación gráfica. Si es grande lo imprimirá en modo texto
 * 
 * @return Status Ejecución de la función
 */
Status plot_vector();

/**
 * @brief Pone a dormir un proceso durante un cierto número de nanosegundos
 * 
 * @param nanoseconds Nanosegundos que debe dormir el proceso
 * @return Status Ejecución de la función
 */
Status fast_sleep(int nanoseconds);

#endif