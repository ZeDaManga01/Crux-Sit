# ![Icon](docs/icon.png) Crux Sit
"Crux Sit" é um jogo desenvolvido em C para a FPGA DE1-SoC com ajuda da GPU CoLenda, onde o jogador é um caçador de seres sobrenaturais e precisa impedir que ondas de monstros alcancem a vila.

O jogo possui três tipos de monstros: zumbis, lobisomens e vampiros, e o jogador precisa atirar neles usando o tipo de munição efetiva para cada monstro: bala normal, bala de prata e dente de alho. Ademais, os monstros possuem atributos diferentes: o zumbi se move lentamente, enquanto o lobisomem corre e o vampiro voa em uma senóide, exigindo que o jogador tenha uma boa precisão de mira para defender a vila. Por fim, o jogador possui três vidas, e perde uma quando um monstro consegue alcançar a vila. Os pontos são acumulados conforme os monstros são derrotados. A rodada atual termina quando amanhece.

Os cenários e sprites dos monstros foram feitos com o auxílio da plataforma online Piskel. O Piskel possui a opção de exportar o sprite atual como um array de cores RGBA em hexadecimal. Assim, para realizar a leitura do array e converter em informações que a biblioteca da GPU CoLenda necessita, foi escrita uma função para converter o número hexadecimal em seus valores RGBA correspondentes (0 até 255 para vermelho, verde, azul e alpha, que indica a transparência do pixel). Aliada à essa função, também foi escrita uma função para normalizar esses valores (dividir cada componente da cor por 32 para caber em 3 bits cada, de 0 até 7).

Os monstros surgem na tela da direita para a esquerda, e o seu tipo (zumbi, lobisomem ou vampiro) é aleatorizado. Então, as informações do monstro são empacotadas em uma estrutura e colocadas em um vetor com suas posições livres sendo rastreadas por um vetor auxiliar, responsável por armazenar se a posição neste vetor principal está ocupada com alguma entidade ou não. A movimentação de cada tipo de monstro foi realizada atribuindo posição e velocidade aleatórias para cada um e checando o tempo percorrido entre uma iteração e outra. Aplicando as fórmulas físicas de posição em relação à velocidade e ao tempo e a de velocidade em relação à aceleração e ao tempo, as posições de todos os monstros é atualizada com a primeira, enquanto a segunda se torna útil para calcular a velocidade atual dos lobisomens e dos vampiros. Como mencionado anteriormente, os vampiros voam em senóides, e os lobisomens correm. Os lobisomens podem acelerar ou desacelerar aleatóriamente, mas o movimento do vampiro possui nuances a mais: isso é alcançado ao definir uma lógica para a velocidade e aceleração vertical do vampiro, utilizando um ponto fixo yo como referência para o eixo vertical.

- Primeiro, uma velocidade vertical aleatória (vy) é atribuída ao vampiro.
- A aceleração vertical do monstro (ay) é definida igual à sua velocidade vertical.
- Um ponto fixo yo é definido, e será usado como eixo de referência para o movimento senoidal.
- Se a posição vertical atual do monstro y for maior que yo e a aceleração vertical ay for positiva, inverte-se o sinal da aceleração para torná-la negativa.
- Se a posição vertical atual do monstro y for menor que yo e a aceleração vertical ay for negativa, inverte-se novamente o sinal da aceleração para torná-la positiva.

Essa lógica faz com que o monstro alterne sua direção vertical sempre que ultrapassa o ponto fixo yo, resultando em um movimento senoidal.

O jogo foi dividido em threads, a fim de tornar a verificação dos botões e movimentos do mouse em paralelo, consequentemente aumentando o desempenho.
