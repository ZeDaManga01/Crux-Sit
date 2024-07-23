# ![Icon](docs/icon.png) Crux Sit
"Crux Sit" é um jogo desenvolvido em C para a FPGA DE1-SoC com ajuda da GPU CoLenda, onde o jogador é um caçador de seres sobrenaturais e precisa impedir que ondas de monstros alcancem a vila.

O jogo apresenta três tipos de monstros: zumbis, lobisomens e vampiros. Cada tipo de monstro possui atributos únicos que influenciam sua movimentação e exigem diferentes tipos de munição para serem derrotados: bala normal para zumbis, bala de prata para lobisomens e dente de alho para vampiros.

Os zumbis movem-se lentamente, enquanto os lobisomens são rápidos e os vampiros exibem um movimento senoidal, desafiando a precisão do jogador. A mecânica do jogo é desenhada de forma a exigir habilidade e estratégia, pois cada monstro precisa ser abatido com a munição correta para evitar que alcancem a vila. O jogador possui três vidas, perdendo uma sempre que um monstro consegue atravessar a defesa. Os pontos são acumulados conforme os monstros são derrotados, e a rodada termina com o amanhecer.

A importância de escrever um driver para a GPU CoLenda reside na capacidade de explorar ao máximo o potencial da FPGA DE1-SoC, unindo o conhecimento adquirido sobre como o hardware influencia o software no produto final.

Os cenários e sprites dos monstros foram feitos com o auxílio da plataforma online Piskel. O Piskel possui a opção de exportar o sprite atual como um array de cores RGBA em hexadecimal. Assim, para realizar a leitura do array e converter em informações que a biblioteca da GPU CoLenda necessita, foi escrita uma função para converter o número hexadecimal em seus valores RGBA correspondentes (0 até 255 para vermelho, verde, azul e alpha, que indica a transparência do pixel). Aliada à essa função, também foi escrita uma função para normalizar esses valores (dividir cada componente da cor por 32 para caber em 3 bits cada, de 0 até 7).

Os monstros surgem na tela da direita para a esquerda, e o seu tipo (zumbi, lobisomem ou vampiro) é aleatorizado. Então, as informações do monstro são empacotadas em uma estrutura e colocadas em um vetor com suas posições livres sendo rastreadas por um vetor auxiliar, responsável por armazenar se a posição neste vetor principal está ocupada com alguma entidade ou não. A movimentação de cada tipo de monstro foi realizada atribuindo posição e velocidade aleatórias para cada um e checando o tempo percorrido entre uma iteração e outra. Aplicando as fórmulas físicas de posição em relação à velocidade e ao tempo e a de velocidade em relação à aceleração e ao tempo, as posições de todos os monstros é atualizada com a primeira, enquanto a segunda se torna útil para calcular a velocidade atual dos lobisomens e dos vampiros. Como mencionado anteriormente, os vampiros voam em senóides, e os lobisomens correm. Os lobisomens podem acelerar ou desacelerar aleatóriamente, mas o movimento do vampiro possui nuances a mais: isso é alcançado ao definir uma lógica para a velocidade e aceleração vertical do vampiro, utilizando um ponto fixo yo como referência para o eixo vertical.

- Primeiro, uma velocidade vertical aleatória (vy) é atribuída ao vampiro.
- A aceleração vertical do monstro (ay) é definida igual à sua velocidade vertical.
- Um ponto fixo yo é definido, e será usado como eixo de referência para o movimento senoidal.
- Se a posição vertical atual do monstro y for maior que yo e a aceleração vertical ay for positiva, inverte-se o sinal da aceleração para torná-la negativa.
- Se a posição vertical atual do monstro y for menor que yo e a aceleração vertical ay for negativa, inverte-se novamente o sinal da aceleração para torná-la positiva.

Essa lógica faz com que o monstro alterne sua direção vertical sempre que ultrapassa o ponto fixo yo, resultando em um movimento senoidal.

O jogo foi dividido em threads, a fim de tornar a verificação dos botões e movimentos do mouse em paralelo, consequentemente aumentando o desempenho.
