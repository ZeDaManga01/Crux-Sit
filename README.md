# <div align="center">![Icon](docs/icon.png) Crux Sit</div>

<div id="resumo" align="justify"> 
<h3 align="center"> Resumo </h3>
Este README possui o objetivo de descrever a estruturação e a elaboração do jogo "Crux Sit", desenvolvido no Kit de Desenvolvimento DE1-SoC com o auxílio da arquitetura da GPU CoLenda, projeto relacionado ao Trabalho de Conclusão de Curso do discente Gabriel Barreto Alves. O projeto de desenvolvimento do jogo se baseou na preparação de um <i>driver</i> de núcleo <i>kernel</i> para implementar a comunicação com os barramentos da GPU CoLenda, além de bibliotecas auxiliares que processam a utilização do mouse, máquinas de estados, deslocamentos de bits e lógicas combinacionais, estruturando o alicerce do código do jogo. A implementação do jogo atendeu a todos os requisitos necessários e propõem um projeto de jogo operante e funcional.
</div>

<div id="abstract" align="justify"> 
<h3 align="center"> Abstract </h3>
This README aims to describe the structuring and development of the game “Crux Sit”, developed in the DE1-SoC Development Kit with the help of the CoLenda GPU architecture, the Final Project of the student Gabriel Barreto Alves. The game development project was based on preparing a kernel driver to implement communication with the CoLenda GPU buses, as well as auxiliary libraries that process the use of the mouse, state machines, bit shifts and combinational logic, structuring the foundation of the game code. The implementation of the game met all the necessary requirements and proposes an operative and functional project.
</div>

<div id="team">
<h2 align="justify"> Equipe </h2>

<uL>
  <li><a href="https://github.com/pierremachado">Pierre Machado</a></li>
  <li><a href="https://github.com/kevincorges">Kevin Borges</a></li>
  <li><a href="https://github.com/ZeDaManga01">José Alberto</a></li>
</ul>
</div>

<div id="intro" align="justify">
<h2> 1. Introdução </h2>

A importância de desenvolver um driver para a GPU CoLenda reside na capacidade de explorar ao máximo o potencial da FPGA DE1-SoC, unindo o conhecimento adquirido sobre como o hardware influencia o software no produto final.

"Crux Sit" é um jogo desenvolvido em C para a FPGA DE1-SoC com ajuda da GPU CoLenda, onde o jogador assume o papel de um caçador de seres sobrenaturais e precisa impedir que ondas de monstros alcancem a vila. O jogo apresenta três tipos de monstros: zumbis, lobisomens e vampiros. Cada tipo de monstro possui atributos únicos que influenciam sua movimentação e exigem diferentes tipos de munição para serem derrotados: bala normal para zumbis, bala de prata para lobisomens e dente de alho para vampiros.

Os zumbis movem-se lentamente, enquanto os lobisomens são rápidos e os vampiros exibem um movimento senoidal, desafiando a precisão do jogador. A mecânica do jogo é desenhada de forma a exigir habilidade e estratégia, pois cada monstro precisa ser abatido com a munição correta para evitar que alcancem a vila. O jogador possui três vidas, perdendo uma sempre que um monstro consegue atravessar a defesa. Os pontos são acumulados conforme os monstros são derrotados, e a rodada termina com o amanhecer.

</div>

<div id="req">
<h2 align="justify"> 2. Requisitos </h2>

O problema a ser desenvolvido no Kit FPGA DE1-SoC deve atender às seguintes restrições:
- O código deve ser escrito em linguagem C;
- O sistema só poderá utilizar os componentes disponíveis na placa;
- Um novo sprite deve ser colocado na memória e utilizado no jogo;
- As ações do ator do jogo (pulo, tiro, etc.) devem ser comandadas pelos botões do mouse;
- A variação da velocidade no movimento do mouse deve ser refletida na ação do ator do jogo. Por exemplo, no jogo breakout a barra se move com velocidade maior se o movimento do mouse for brusco;
- Informações do jogo (placar, vidas, etc.) devem ser exibidas no display de 7-segmentos;
- O jogo deve permitir ações do usuário através dos botões da DE1-SoC, no mínimo: a pausa, o retorno, o reinício e o término do jogo.
- O usuário poderá parar e reiniciar o jogo em qualquer momento; 
- O usuário poderá sair do jogo em qualquer momento.
- Pelo menos um elemento passivo do jogo deverá se mover.

</div>


<div id="metodo" align="justify"> 
<h2> 3. Metodologia </h2>

  Esta seção visa expor o processo de criação do jogo "Crux Sit" e a sua implementação no Kit de Desenvolvimento DE1-SoC.

  <div id="sft_ut" align="justify"> 
  <h3> 3.1. Softwares Utilizados</h3>

  #### 3.1.1. C

  A linguagem C é amplamente utilizada em projetos devido à sua eficiência e versatilidade. Com sua sintaxe direta e controle próximo sobre o hardware, o C permite desenvolver programas robustos e rápidos, especialmente em sistemas embarcados, drivers de dispositivos e software de baixo nível. No contexto deste projeto, a utilização da linguagem C foi um dos requisitos necessarios a serem cumpridos.

  #### 3.1.2. VS Code

  O Visual Studio Code (VS Code) é uma ferramenta popular e altamente funcional utilizada em uma variedade de projetos de desenvolvimento de software. O uso dele foi necessário para agilizar o desenvolvimento, permitindo editar, depurar e gerenciar o código de maneira simplificada e eficaz.

  #### 3.1.3. GNU/Linux

  Por fim, o kit de desenvolvimento DE1-SoC possui uma distribuição do Linux embarcado instalada, possibilitando a comunicação com o kit bem como a execução dos códigos criados através de conexão remota. Isso oferece uma gama de possibilidades para a elaboração do problema: a disposição dos diretórios do sistema e a possibilidade de compilar códigos na linguagem de programação requisitada de forma fácil com o compilador GCC embarcado no kit de desenvolvimento foram fundamentais.

  </div>


  <div id="kit_placa" div align="justify"> 
  <h3> 3.2. Kit de Desenvolvimento DE1-SoC</h3>

  O kit de desenvolvimento DE1-SoC apresenta uma plataforma robusta de design de hardware construída em torno do Altera FPGA System-on-Chip (SoC), que combina os mais recentes núcleos incorporados Cortex-A9 dual-core com lógica programável líder do setor para máxima flexibilidade de projeto.

  <p align="center">
    <img src="https://github.com/ZeDaManga01/PBL-01-MI---Sistemas-Digitais/blob/main/docs/image%20-%20De1-SoC.jpeg" />
  </p>

  #### 3.2.1. ARM CORTEX A9

  A arquitetura utilizada por esse processador é a RISC – Reduced Instruction Set Computer. Sua operações lógicas e aritméticas são efetuadas em operadores dos registradores de propósito geral. Os dados são movidos entre a memória e esses registradores por meio de instruções de carga e armazenamento - Load and Store.

  #### 3.2.2. Registradores

  O ARM possui 16 registradores de 32 bits, sendo 15 de uso geral, R0 a R14 e um Program Counter R15. O registrador R15 tem o endereço da próxima instrução que será executada. Os registradores R13 e R14, são usados convencionalmente como ponteiro de pilha Stack Pointer, que contém o endereço atual do elemento superior da pilha e registrador de link Link Register, que recebe o endereço de retorno em chamadas de procedimento, respectivamente.

  #### 3.2.3. Memória

  A placa suporta 1GB de SDRAM DDR3, compreendendo dois dispositivos DDR3 de 16 bits cada no lado do HPS. Os sinais estão conectados ao Controlador de Memória Dedicado para os bancos de I/O do HPS e a velocidade alvo é de 400 MHz.

  #### 3.2.4. Diagrama de Blocos da DE1-SoC

  Todas as conexões são estabelecidas através do dispositivo Cyclone V SoC FPGA para fornecer flexibilidade máxima aos usuários. Os usuários podem configurar o FPGA para implementar qualquer projeto de sistema.

  <p align="center">
    <img src="https://github.com/ZeDaManga01/PBL-01-MI---Sistemas-Digitais/blob/main/docs/Image.jpeg" />
  </p>

  #### 3.2.5. Gigabit Ethernet
  
  A placa suporta transferência Gigabit Ethernet por um chip externo Micrel KSZ9021RN PHY e função HPS Ethernet MAC. O chip KSZ9021RN com Gigabit 10/100/1000 Mbps integrado. O transceptor Ethernet também suporta interface RGMII MAC.

  </div>


  <div id="sft_ut" align="justify"> 
  <h3> 3.3. Componentes Utilizados</h3>

  #### 3.3.1. GPU CoLenda

  Unidade de processamento gráfico, também conhecida como GPU, é um componente eletrônico projetado para acelerar tarefas relacionadas à computação gráfica e ao processamento de imagens em uma ampla gama de dispositivos, incluindo placas de vídeo, placas-mãe, smartphones e computadores pessoais (PCs). A capacidade da GPU de realizar cálculos matemáticos complexos de forma rápida e eficiente reduz significativamente o tempo necessário para que um computador execute uma variedade de programas.

  O discente Gabriel Barreto Alves foi responsável pelo projeto de elaboração da CoLenda, GPU utilizada no projeto. A CoLenda é capaz de renderizar, em uma tela de resolução 640x480, dois tipos de polígonos convexos (quadrado e triângulo), sprites, planos de fundo e blocos de pixel 8x8, possibilitando a seleção de cor com 3 bits para vermelho, verde e azul para os dois últimos e para os pixels dos sprites. Há a possibilidade de utilizar quatro tipos de instruções diferentes, e foi desenvolvido um driver de módulo kernel a fim de gerenciar estas instruções com o uso de uma biblioteca, cuja metodologia de desenvolvimento pode ser acessada através [<u>deste link</u>](https://github.com/ZeDaManga01/PBL-02-MI---Sistemas-Digitais).

  #### 3.3.2. Monitor CRT

  O monitor utilizado no projeto foi o DELL M782p, um modelo CRT. Esse tipo de monitor utiliza um tubo de raios catódicos (CRT) para exibir imagens. O DELL M782p possui uma tela de visualização de 17 polegadas e uma resolução máxima de 1280x1024 pixels. Ele oferece uma interface VGA para conexão com o computador ou placa de desenvolvimento. Os monitores CRT são conhecidos por sua reprodução de cores vibrantes e tempos de resposta rápidos, tornando-os uma escolha adequada para projetos que exigem interação em tempo real, como jogos e simulações.

  </div>


  <div id="sprite" align="justify"> 
  <h3> 3.4. Criação dos cenários e sprites </h3>

  Os cenários e sprites dos monstros foram criados utilizando a plataforma online Piskel, que permite a exportação dos sprites como vetores de cores RGBA codificadas em hexadecimal. Para ler esses arrays e convertê-los em informações compatíveis com a biblioteca da GPU CoLenda, foi desenvolvida uma função específica. Esta função converte os números hexadecimais em seus valores correspondentes de RGBA (variando de 0 a 255 para vermelho, verde, azul e alfa, que indica a transparência do pixel). Além disso, foi implementada uma função de normalização para ajustar esses valores, dividindo cada componente da cor por 32 para que se encaixem em 3 bits cada (variando de 0 a 7).

  </div>

  <div id="code" align="justify"> 
  <h3> 3.5. Captura de eventos do mouse </h3>

  

  </div>

  <div id="mov" align="justify"> 
  <h3> 3.6. Movimentação </h3>

  Os monstros aparecem na tela movendo-se da direita para a esquerda, com o tipo de monstro (zumbi, lobisomem ou vampiro) sendo aleatorizado. As informações de cada monstro são então empacotadas em uma estrutura e armazenadas em um vetor, com suas posições livres sendo monitoradas por um vetor auxiliar, que registra se uma posição no vetor principal está ocupada por uma entidade ou não.

  A movimentação dos monstros é gerida atribuindo posições e velocidades aleatórias a cada um e verificando o tempo decorrido entre as iterações. Utilizando fórmulas físicas de posição em relação à velocidade e ao tempo, assim como de velocidade em relação à aceleração e ao tempo, as posições de todos os monstros são atualizadas. A primeira fórmula é aplicada para atualizar as posições, enquanto a segunda é utilizada para calcular a velocidade atual dos lobisomens e vampiros.

  Como mencionado anteriormente, os vampiros voam em senóides, e os lobisomens correm. Os lobisomens podem acelerar ou desacelerar aleatóriamente, mas o movimento do vampiro possui nuances a mais: isso é alcançado ao definir uma lógica para a velocidade e aceleração vertical do vampiro, utilizando um ponto fixo yo como referência para o eixo vertical.

  - Primeiro, uma velocidade vertical aleatória (vy) é atribuída ao vampiro.
  - A aceleração vertical do monstro (ay) é definida igual à sua velocidade vertical.
  - Um ponto fixo yo é definido, e será usado como eixo de referência para o movimento senoidal.
  - Se a posição vertical atual do monstro y for maior que yo e a aceleração vertical ay for positiva, inverte-se o sinal da aceleração para torná-la negativa.
  - Se a posição vertical atual do monstro y for menor que yo e a aceleração vertical ay for negativa, inverte-se novamente o sinal da aceleração para torná-la positiva.

  Essa lógica faz com que o monstro alterne sua direção vertical sempre que ultrapassa o ponto fixo yo, resultando em um movimento senoidal.

  </div>


  <div id="code" align="justify"> 
  <h3> 3.7. Separação em threads </h3>

  O jogo foi dividido em threads, a fim de tornar a verificação dos botões e movimentos do mouse em paralelo, consequentemente aumentando o desempenho.

  </div>


  <div id="code" align="justify"> 
  <h3> 3.8. Código </h3>

  

  </div>

</div>


<div id="dinamic"> 
<h2> Dinâmica do jogo</h2>
<div align="justify">

<div id="test"> 
<h2> Testes</h2>
<div align="justify">

Esta sessão é reservada para demonstração dos testes feitos no projeto.


<div id="resultconcl" align="justify"> 
<h2> Resultados e Conclusão</h2>

Conclui-se que o projeto foi implementado de forma satisfatória. O driver e a biblioteca se mostraram funcionais e atenderam aos requisitos propostos. Ademais, foi essencial para expandir o conhecimento acerca do kit de desenvolvimento, GNU/Linux embarcado e a comunicação hardware/software, contribuindo para a sofisticação de projetos futuros a serem implementados no kit de desenvolvimento DE1-SoC.
</div>


<div id="makefile"> 
<h2> Instruções para executar o programa</h2>
<div align="justify">

Abrindo a pasta do projeto no terminal, escreva:

```
cd CoLendaDriver
sudo make install
```

Siga as instruções mostradas no fim da instalação do driver para a criação do arquivo nó.

Então, volte para o diretório do projeto, e escreva:

```
make
sudo ./main
```

Para desinstalar o driver, com o diretório do projeto aberto no terminal, escreva:

```
cd CoLendaDriver
sudo make uninstall
```

<div id="ref"> 
<h2> Referências </h2>
<div align="justify">
  
DE1-SoC Board. Disponível em: https://www.terasic.com.tw/cgi-bin/page/archive.pl?Language=English&No=836&PartNo=4. Acessado em: 7 de maio de 2024.

Introduction to the ARM Cortex-A9 Processor. Disponível em: https://github.com/fpgacademy/Tutorials/releases/download/v21.1/ARM_intro_intelfpga.pdf. Acessado em: 5 de maio de 2024.

TANENBAUM, Andrew S. Sistemas Operacionais Modernos. 4. ed. São Paulo: Pearson Prentice Hall, 2008. p. 504.
</div>
