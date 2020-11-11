/**
 * @file Sort.h
 * @author Kevin de la Coba Malam (kevin.coba@estudiante.uam.es)
 *         Jose Manuel Freire     (jose.freire@estudiante.uam.es)
 * @brief Archivo donde se definen los datos del modulo principal del programa
 * 
 * Modulo: Sort
 * 
 * @version 1
 * @date 2020-04-28
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef SORT_H
#define SORT_H

#include "Global.h"
#include "Utils.h"

/**
 * @brief Enumeración que representa el estado de una tarea
 * 
 */
typedef enum {
    Incompleta,
    Enviada,
    EnProceso,
    Completada
} Completed;

/**
 * @brief Estrcutura que define una tarea
 * 
 */
typedef struct {
    Completed estado;   //Estado de la tarea
    int ini;            //Posición inicial de la tarea (con respecto al array)
    int end;            //Posición final de la tarea
    int mid;            //Posición intermedia de la tarea
} Task;

/**
 * @brief Estructura principal que contiene los datos del sistema de ordenación
 * 
 */
typedef struct {
    Task **tareas;  //Doble puntero para guardar las tareas por nivel
    int *array;     //Array a ordenar
    int longitud;   //Longitud del array a ordenar
    int retardo;    //Retardo en el proceso de ordenación para poder visualizarlo
    int niveles;    //Niveles del programa
    int procesos;   //Número de procesos para ordenar
    pid_t padre;    //Pid del proceso principal del sistema
} Sort;

//Prototipo de funciones
/**
 * @brief Ordena un vector utilizando el 
 * algoritmo bubbleSort
 * 
 * Al proceso se le asigna una tarea, y en esa tarea tiene especificado
 * que partes del array ordenar
 * 
 * @return Status Ejecución de la función
 */
Status bubble_sort();

/**
 * @brief Ordena un vector utilizando el
 * algoritmo merge.
 * 
 * Al proceso se le asigna una tarea, y en esa tarea tiene especificado
 * que partes del array ordenar
 * 
 * @param tarea Tarea con los indices del array
 * @param vector Vector a ordenar
 * @param retardo Retardo tras cada comparación
 * @return Status Ejecución de la función
 */
Status merge(Task *tarea, int *vector, int retardo);

/**
 * @brief Devulve el número de tareas en un nivel
 * 
 * @param tareas Tareas del programa
 * @param nivel Nivel a consultar
 * @return int Numero de tareas en ese nivel
 */
int get_number_parts(Task **tareas, int nivel);

/**
 * @brief Inicializa la estructura de tipo Sort
 * 
 * @param pf Archivo con la información
 * @param s Estructura a inicializar
 * @param niveles Niveles del programa
 * @param procesos Totales del programa
 * @param retardo Retardo para cada comparacion
 * @return Status Ejecucion de la función
 */
Status init_sort(FILE *pf, Sort *s, int niveles, int procesos, int retardo);

/**
 * @brief Destruye la estructura de tipo sort
 * 
 * @param s Estructura a destruir
 * @return Status Ejecución de la función
 */
Status destroy_sort(Sort *s);

/**
 * @brief Indica si una tarea está lista para ser abordada
 * 
 * @param tareas Tareas del sistema
 * @param tarea Tarea a comprobar
 * @return Bool TRUE si es posible el abordaje, FALSE si no lo es
 */
Bool check_task_ready(Task **tareas, Task* tarea);
//TODO Para esta funcion hara falta una función privada que compare tareas

/**
 * @brief Resuelve la tarea indicada por el nivel y la parte dentro de este
 * 
 * @param s Estructura del programa donde se encuentra el array a resolver
 * @param nivel Nivel donde se encuentra la tarea a resolver
 * @param parte Parte dentro del nivel donde se encuentra la tarea a resolver
 * @return Status Ejecución de la función
 */
Status solve_task(Sort *s, int nivel, int parte); 

/**
 * @brief Funcion que inicializa el sistema y ordena el array utilizando un único proceso
 * 
 * @return Status Ejecución de la función
 */
Status sort_single_process();

#endif