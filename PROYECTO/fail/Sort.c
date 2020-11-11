/**
 * @file Sort.c
 * @author Kevin de la Coba Malam (kevin.coba@estudiante.uam.es)
 *         Jose Manuel Freire     (jose.freire@estudiante.uam.es)
 * @brief 
 * @version 0.1
 * @date 2020-04-28
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "Sort.h"

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
Status merge(Task *tarea, int *vector, int retardo) {
    int i = tarea->ini,
        n = tarea->mid,
        k = 0, j= 0,
        *vector_aux = NULL;

    if(tarea == NULL || tarea == NULL || vector == NULL) return ERR;
    if (tarea->estado != Enviada) return ERR;

    //Cambiamos el estado de la tarea a en proceso
    tarea->estado = EnProceso;

    //Reservamos memoria para el vector auxiliar
    vector_aux = (int*)malloc(((tarea->mid-tarea->ini)+(tarea->end-tarea->mid+1)+2)*sizeof(int));
    if (vector_aux == NULL) {
        perror("merge: malloc");
    }

    while(tarea->mid-1 >= i && tarea->end-1 >= n) {
        
        //Comparamos
        if (vector[i] < vector[n]) {
            vector_aux[k] = vector[i];
            k++;i++;
        } else {
            vector_aux[k] = vector[n];
            k++;n++;
        }

        //Añadimos retardo
        fast_sleep(retardo);
    }

    //Copiando lo que falta del array que no se haya copiado entero
    if (i == tarea->mid) {
        while(n <= tarea->end-1) {
            vector_aux[k] = vector[n];
            k++;n++;
        }
    } else if (n == tarea->end) {
        while(i <= tarea->mid-1) {
            vector_aux[k] = vector[i];
            k++;i++;
        }
    }

    //Copiando el vector auxiliar en el vector origen
    k = 0;
    for (int i = tarea->ini; i < tarea->end; i++, k++)
        vector[i] = vector_aux[k];

    free(vector_aux);
}

/**
 * @brief Inicializa la estructura de tipo Sort
 * 
 * @param pf Archivo con la información
 * @param s Estructura a inicializar
 * @param niveles Niveles del programa
 * @param procesos Totales del programa
 * @param retardo Retardo para cada comparacion
 * @return Ejecucion de la función
 */
Status init_sort(FILE *pf, Sort *s, int niveles, int procesos, int retardo) {

    int *array = NULL;
    int len = 0;
    int num = 0;
    int tareas = 0;

    if (pf == NULL || s == NULL || niveles < 0 || procesos < 0 || retardo < 0) return ERR;

    //Reservamos memoria para la estructura
    s = (Sort*)malloc(sizeof(Sort));
    if (s == NULL) {
        perror("init_sort: malloc");
        return ERR;
    }

    // Leemos el numero de digitos a introducir 
    fscanf(pf, "%d\n", &len);

    //Reservamos memoria para el array
    s->array = (int*)malloc(len*sizeof(int));
    if (s->array == NULL) {
        perror("init_sort: malloc");
        free(s);
        return ERR;
    }

    // Leemos el array y lo introducimos en la estructura 
    num = len;
    for(int i = 0; i < num; i++) fscanf(pf, "%d\n", &s->array[i]);

    // Introducimos los datos en la estructura 
    s->niveles = niveles;
    s->procesos = procesos;
    s->retardo = retardo;

    //Reservamos memoria para las tareas
    s->tareas = (Task**)malloc(niveles*sizeof(Task*));
    if (s->tareas == NULL) {
        perror("init_sort: malloc");
        free(s->array);
        free(s);
        return ERR;
    }

    //Por cada nivel debemos crear las tareas necesarias
    //Primero el nivel 0 que tendrá tantas tareas como procesos
    s->tareas[0] = (Task*)malloc(procesos*sizeof(Task));
    if (s->tareas[0] == NULL) {
        perror("init_sort: malloc");
        free(s->array);
        free(s);
        return ERR;
    }

    //Reservamos memoria para las tareas restantes
    tareas = procesos;
    for(int i = 1; i < niveles; i++) {

        //Cada vez las tareas son menores
        tareas = compute_log(tareas);
        s->tareas[i] = (Task*)malloc(tareas*sizeof(Task));
        if (s->tareas[i] == NULL) {
            perror("init_sort: malloc");
            for(int n = 0; n < i; n++) free(s->tareas[n]);
            free(s->tareas);
            free(s->array);
            free(s);
            return ERR;
        }
    }

    //Inicializamos las tareas a incompletas
    tareas = procesos;
    for (int i = 0; i < niveles; i++) {

        for (int n = 0; n < tareas; n++) 
            s->tareas[i][n].estado = Incompleta;
        
        tareas = compute_log(tareas);
    }

    //Asignamos los datos a las tareas
    tareas = procesos;
    num = len/procesos;
    if (procesos > len) {
        printf("Demasiados procesos\n");
        for(int n = 0; n < niveles; n++) free(s->tareas[n]);
        free(s->tareas);
        free(s->array);
        free(s);
        return ERR;
    }
    for (int i = 0; i < niveles; i++) {

        for (int n = 0; n < tareas; n++) {
            
        }
        tareas = compute_log(tareas);
    }

    return OK;
}

/**
 * @brief Destruye la estructura de tipo sort
 * 
 * @param s Estructura a destruir
 * @return Status Ejecución de la función
 */
Status destroy_sort(Sort *s) {
    if (s == NULL) return ERR;

    free(s->array);
    for (int i = 0; i < s->niveles; i++)free(s->tareas[i]);
    free(s->tareas);
    free(s);

    return OK;
}

/**
 * @brief Resuelve la tarea indicada por el nivel y la parte dentro de este
 * 
 * @param s Estructura del programa donde se encuentra el array a resolver
 * @param nivel Nivel donde se encuentra la tarea a resolver
 * @param parte Parte dentro del nivel donde se encuentra la tarea a resolver
 * @return Status Ejecución de la función
 */
Status solve_task(Sort *s, int nivel, int parte) {
    if (s == NULL || nivel < 0 || parte < 0) return ERR;

    /* Comprobamos si la tarea tiene que hacer merge o bublesort */
    if (s->tareas[nivel][parte].ini < s->tareas[nivel][parte].mid &&
        s->tareas[nivel][parte].mid < s->tareas[nivel][parte].end) { //MergeSort
       
        //Cambiamos el estado de la tarea a enviada
        s->tareas[nivel][parte].estado = Enviada;
        merge(&s->tareas[nivel][parte], s->array, s->retardo);
        return OK;
    } else { //BubbleSort

        //Cambiamos el estado de la tarea a enviada
        s->tareas[nivel][parte].estado = Enviada;
        bubble_sort(&s->tareas[nivel][parte], s->array, s->retardo);
        return OK;
    }
}

void main() {

    int vector[20];
    Task t1;
    FILE *pf;
    Sort *s;

    pf = fopen("a", "r");

    init_sort(pf, s, 2, 4, 10);

    t1.end = 10;
    t1.ini = 2;
    t1.mid = 6;
    t1.estado = Enviada;


    for(int i = 0; i < 20; i++) vector[i] = 0;
    vector[2] = 1;
    vector[3] = 3;
    vector[4] = 4;
    vector[5] = 6;
    vector[6] = 2;
    vector[7] = 5;
    vector[8] = 7;
    vector[9] = 8;
    vector[10] =  -1;
    vector[16] = -1;
    for(int i = 0; i < 20; i++) printf("%d ", vector[i]);
    printf("\n");


    merge(&t1, vector, 100000);

    for(int i = 0; i < 20; i++) printf("%d ", vector[i]);
    printf("\n");

}