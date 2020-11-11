/**
 * @file Global.h
 * @author Kevin de la Coba Malam (kevin.coba@estudiante.uam.es)
 *         Jose Manuel Freire     (jose.freire@estudiante.uam.es)
 * @brief Archivo con las definiciones de dos tipos de datos que se utilizan
 * como retorno de las funciones. 
 * 
 * Modulo: Global
 * 
 * @version 1
 * @date 2020-04-28
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef GLOBAL_H
#define GLOBAL_H

//Librerias comunes entre los modulos
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/**
 * @brief Enumeración que indica la forma en la que una función acaba
 * 
 */
typedef enum {
    ERR,        //Por si se produce un error
    OK          //Para cuando la función acabe correctamente
} Status;

/**
 * @brief Enumeracion para representar un dato booleano
 * 
 */
typedef enum {
    FALSE,
    TRUE
} Bool;

#endif