#include <stdio.h>
#include <string.h>

typedef enum
{
    e0,
    e1,
    e2,
    e3,
    e4,
    e5,
    e6,
    e7,
    e8,
    e9,
    e10,
    eg,
    dev,
    q1,
    q2,
    q5
} state_type;

typedef struct
{
    int a, b, c; // Monedas
    int y, B, reset;
    int clock;
} input_type;

typedef struct
{
    int entrega;
    int dev_100;
    int dev_200;
    int dev_500;
} output_type;

const char *estado_nombres[] = {
    "e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7", "e8", "e9", "e10",
    "eg", "dev", "q1", "q2", "q5"};

int saldo = 0;
int devolviendo = 0;
int devolucion_por_solicitud = 0;

state_type fsm_transition(state_type estado, input_type in)
{
    if (in.reset)
    {
        saldo = 0;
        devolviendo = 0;
        devolucion_por_solicitud = 0;
        return e0;
    }

    if (!in.clock)
        return estado;

    // Solicitud de devolución manual
    if (in.y && estado >= e1 && estado <= e10)
    {
        devolviendo = saldo;
        devolucion_por_solicitud = 1;
        return dev;
    }

    // Entrega de producto
    if (estado == e10 && in.B)
    {
        saldo = 0;
        return eg;
    }

    // Desborde en e10: moneda extra provoca devolución automática
    if (estado == e10 && (in.a || in.b || in.c))
    {
        // cálculo de exceso ingresado
        devolviendo = in.a * 100 + in.b * 200 + in.c * 500;
        devolucion_por_solicitud = 0;
        return dev;
    }

    // Máquina de devolución paso a paso
    if (estado == dev)
    {
        if (devolviendo >= 500)
        {
            devolviendo -= 500;
            return q5;
        }
        else if (devolviendo >= 200)
        {
            devolviendo -= 200;
            return q2;
        }
        else if (devolviendo >= 100)
        {
            devolviendo -= 100;
            return q1;
        }
        else
        {
            // Al terminar la devolución:
            if (devolucion_por_solicitud)
            {
                saldo = 0;
                devolucion_por_solicitud = 0;
                return e0;
            }
            else
            {
                devolucion_por_solicitud = 0;
                return e10;
            }
        }
    }

    if (estado == q1 || estado == q2 || estado == q5)
        return dev;

    if (estado == eg)
        return e0;

    // Acumulación de monedas en estados e0-e9
    if (estado >= e0 && estado <= e9)
    {
        int ingreso = in.a * 100 + in.b * 200 + in.c * 500;
        if (ingreso == 0)
            return estado;

        saldo += ingreso;

        if (saldo > 1000)
        {
            int exceso = saldo - 1000;
            saldo = 1000;
            devolviendo = exceso;
            devolucion_por_solicitud = 0;
            return dev;
        }

        return (state_type)(saldo / 100);
    }

    return estado;
}

void fsm_output(state_type estado, output_type *out)
{
    out->entrega = (estado == eg);
    out->dev_100 = (estado == q1);
    out->dev_200 = (estado == q2);
    out->dev_500 = (estado == q5);
}

int main()
{
    state_type estado = e0;
    input_type in;
    output_type out;
    char linea[32];

    while (1)
    {
        memset(&in, 0, sizeof(input_type));

        printf("\nEstado actual: %s | Saldo: $%d\n", estado_nombres[estado], saldo);
        printf("Ingrese entrada (letras: a b c y B r): ");
        fgets(linea, sizeof(linea), stdin);

        for (int i = 0; i < strlen(linea); i++)
        {
            switch (linea[i])
            {
            case 'a':
                in.a = 1;
                break;
            case 'b':
                in.b = 1;
                break;
            case 'c':
                in.c = 1;
                break;
            case 'y':
                in.y = 1;
                break;
            case 'B':
                in.B = 1;
                break;
            case 'r':
                in.reset = 1;
                break;
            default:
                break;
            }
        }

        in.clock = 1;

        state_type anterior = estado;
        estado = fsm_transition(estado, in);
        fsm_output(estado, &out);

        if (estado != anterior)
            printf(">> Transicion: %s -> %s\n", estado_nombres[anterior], estado_nombres[estado]);

        if (out.entrega)
            printf(">> Producto ENTREGADO\n");

        // Mostrar transición de entrega a e0
        if (estado == eg)
        {
            printf(">> Transicion: eg -> e0\n");
            estado = e0;
        }

        // Ciclo automático de devolución completa con trazas
        while (estado == dev || estado == q1 || estado == q2 || estado == q5)
        {
            anterior = estado;
            estado = fsm_transition(estado, (input_type){.clock = 1});
            fsm_output(estado, &out);

            printf(">> Transicion: %s -> %s\n", estado_nombres[anterior], estado_nombres[estado]);

            if (estado == q1)
                printf(">> Devuelto $100\n");
            if (estado == q2)
                printf(">> Devuelto $200\n");
            if (estado == q5)
                printf(">> Devuelto $500\n");

            if (estado == e0 || estado == e10)
                break;
        }

        // No reiniciar nuevamente si ya fue eg
    }

    return 0;
}
