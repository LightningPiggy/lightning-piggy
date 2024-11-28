// Array of Bitcoin slogans
String bitcoinSlogans_pt_BR[] = {
    "O Bitcoin é ouro de nerd.",
    "Nós confiamos no Bitcoin.",
    "Seja seu próprio banco.",
    "Guarde suas moedas (HODL).",
    "Não são as suas chaves, não são as suas moedas.",
    "Existem 100.000.000 de sats em um Bitcoin.",
    "O Bitcoin foi previsto por Henry Ford, Friedrich Hayek e Milton Friedman.",
    "Faça sua própria pesquisa (DYOR).",
    "Pense por si mesmo.",
    "Não delegue sua capacidade de pensar para outro.",
    "Persiga a verdade com determinação.",
    "Ame o próximo como a si mesmo.",
    "O Bitcoin está certo.",
    "Seja correto, justo e gentil.",
    "Se você tem medo de algo, estude-o.",
    "Todos têm um coração faminto.",
    "Todos querem a mesma coisa por motivos igualmente válidos.",
    "Não importa quantas coisas você tenha, será igualmente feliz.",
    "Ter mais não fará você feliz.",
    "Pratique compaixão; coloque-se no lugar de alguém.",
    "Pratique atenção; sinta sem julgamento. Experimente com curiosidade.",
    "Pratique gratidão; sempre encontre coisas em que agradecer.",
    "Ações falam mais alto do que palavras.",
    "O que você faz determina quem você é.",
    "Você tem o direito à propriedade.",
    "Seu corpo é sua propriedade.",
    "Você é o único responsável por si mesmo.",
    "Sua vida é a sua e somente sua.",
    "Somente você pode viver sua vida.",
    "Os ciberpunk escrevem código.",
    "'Se não me acreditar ou não entender, não tenho tempo para tentar convencê-lo, desculpe.' - Satoshi Nakamoto",
    "'O calor do seu computador não é desperdiçado se precisar de aquecer sua casa.' - Satoshi Nakamoto",
    "'O que acham da letra B com as duas linhas a atravessando?' - Satoshi Nakamoto",
    "'A credencial que estabelece alguém como real é a capacidade de fornecer energia ao processador (CPU).' - Satoshi Nakamoto",
    "'Para uma maior privacidade, é melhor usar endereços bitcoin apenas uma vez.' - Satoshi Nakamoto",
    "'Se perder moedas somente faz com que os outros moedas valham ligeiramente mais. Pense nisso como uma doação a todos.' - Satoshi Nakamoto",
    "'Seja humilde, acumule sats.' - Matt Odell",
    "'O Bitcoin é um jogo estranho onde o único movimento vencedor é jogar.' - Bitstein",
    "'Se a privacidade for proibida, apenas os marginais terão privacidade.' - Phil Zimmermann"};

// Function to get a random Bitcoin slogan from the array
String getRandomBootSlogan()
{
  return getRandomElementFromArray(bitcoinSlogans_pt_BR, sizeof(bitcoinSlogans_pt_BR) / sizeof(bitcoinSlogans_pt_BR[0]));
}
