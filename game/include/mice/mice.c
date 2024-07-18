#include "mice.h"

/**
 * @brief Inicializa a estrutura Mice para leitura de dados do mouse.
 * 
 * Essa função abre o dispositivo de mouse "/dev/input/mice" em modo de leitura não-bloqueante.
 * Inicializa a estrutura Mice com valores padrão e retorna a estrutura inicializada.
 *
 * @return Uma estrutura do tipo Mice inicializada com valores padrão.
 * @note A função abre o dispositivo de mouse usando a função open().
 * @note A função inicializa a estrutura Mice com valores padrão, incluindo o descritor de arquivo, dados, e estados dos botões.
 */
Mice mice_open()
{
    Mice mice = {
        .fd = 0,
        .data = {0, 0, 0},
        .dx = 0,
        .dy = 0,
        .event = false,
        .left_press = false,
        .right_press = false,
        .middle_press = false,
        .previous_left_press = false,
        .previous_right_press = false,
        .previous_middle_press = false,
        .left_release = false,
        .right_release = false,
        .middle_release = false
    };

    const char *pDevice = "/dev/input/mice";
    
    /* Trecho de código que abre o mouse. */
    mice.fd = open(pDevice, O_RDONLY | O_NONBLOCK);

    return mice;
}

void mice_close(struct Mice *mice)
{
    close(mice->fd);
    mice->fd = 0; 
}

/**
 * @brief Lê as informações do mouse.
 * 
 * Essa função lê as informações do mouse a partir do descritor de arquivo fornecido.
 * Ela lê os dados do mouse, armazena-os no array de dados e preenche a estrutura Mice com as informações relevantes.
 *
 * @param fd O descritor de arquivo do mouse.
 * @param data Um array de caracteres onde os dados do mouse serão armazenados.
 * @return Uma estrutura do tipo Mice contendo as informações do mouse.
 * @note A função lê 3 bytes do descritor de arquivo para obter as informações do mouse.
 * @note A função preenche a estrutura Mice com os dados lidos, incluindo o evento, deslocamento horizontal e vertical, e os botões pressionados.
 */
void mice_read(Mice *mice)
{   
    memset(mice->data, 0, sizeof(mice->data));

    int bytes = read(mice->fd, mice->data, sizeof(mice->data));

    mice->event = bytes > 0 || mice->left_release || mice->right_release || mice->middle_release;

    mice->dx = mice->event ? mice->data[1] : 0;
    mice->dy = mice->event ? mice->data[2] : 0;

    mice->previous_left_press = mice->left_press;
    mice->previous_right_press = mice->right_press;
    mice->previous_middle_press = mice->middle_press;

    mice->left_press = mice->event && !(mice->dx || mice->dy) ? (int) (mice->data[0] & 0x1) != 0 : mice->left_press;
    mice->right_press = mice->event && !(mice->dx || mice->dy) ? (int) (mice->data[0] & 0x2) != 0 : mice->right_press;
    mice->middle_press = mice->event && !(mice->dx || mice->dy) ? (int) (mice->data[0] & 0x4) != 0 : mice->middle_press;

    mice->left_release = mice->previous_left_press && !mice->left_press;
    mice->right_release = mice->previous_right_press && !mice->right_press;
    mice->middle_release = mice->previous_middle_press && !mice->middle_press;
}

/**
 * @brief Corrige a sensibilidade dos movimentos do mouse.
 * 
 * Essa função ajusta a sensibilidade dos movimentos do mouse dividindo os 
 * dados brutos de movimento do mouse pelos divisores especificados. 
 * O resultado é então arredondado para o número inteiro mais próximo.
 *
 * @param *mice Um ponteiro para a estrutura Mice que contém os dados brutos de movimento do mouse.
 * @param x_divider O divisor para o movimento horizontal do mouse.
 * @param y_divider O divisor para o movimento vertical do mouse.
 * @return Não retorna nada.
 * @note Essa função modifica os campos dx e dy da estrutura Mice.
 * @note A função usa a função ceil() para movimento positivo e a função floor() para movimento negativo.
 */
void correct_mice_sensitivity(struct Mice *mice, int x_divider, int y_divider)
{
    double dx_new = (double) mice->dx / x_divider;
    double dy_new = (double) mice->dy / y_divider;

    mice->dx = mice->dx >= 0 ? (int) ceil(dx_new) : (int) floor(dx_new); 
    mice->dy = mice->dy >= 0 ? (int) ceil(dy_new) : (int) floor(dy_new);

    return;
}
