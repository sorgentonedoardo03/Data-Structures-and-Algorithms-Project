#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
TO-DO:
1-funzione delivery ok memoria ma non ordina bene
2-disponib_ingrediente prende troppo tempo, >=64%.Rifornimento(che dipende da questa) prende il 15%.
  (misurazioni fatte su open11)
RIGA CHE DA PROBLEMI CON TEMPO: esito = disponib_ingrediente(ing->nome_ing, ing->quantita * quantita_dolci); in controllaIngredienti, dentro il while
*/

//=====COSTANTI====
#define MAX_NUM_INGREDIENTI 300000
#define ATTESA 1
#define PRONTO 0
#define DIM_RICETTE 300000

//====STRUTTURE DATI=====

// per gestire il magazzino ingredienti utilizzo una tabella hash in cui ogni elemento di tipo
// ingrediente ha una coda di lotti, in modo che quando devo estrarre un elemento, prendo subito quello
// con la data di scadenza più vicina
typedef struct lotto
{
  int quantita;
  int scadenza;
  struct lotto *next;
} lotto;

typedef struct ingrediente
{
  char nome[80];
  lotto *lotti;
  struct ingrediente *next;
} ingrediente;

typedef struct ing_ricetta
{
  int quantita;
  char nome_ing[80];
  struct ing_ricetta *next;
} ing_ricetta;

typedef struct ricetta
{
  char nome[80];
  int peso;
  ing_ricetta *ingredienti;
  struct ricetta *next;
} ricetta;

typedef struct s_ordine
{
  int data;
  char ricetta[80];
  int quantita;
  int peso;
  struct s_ordine *next;
  struct s_ordine *prev;
} s_ordine;

typedef struct queue_ordini
{
  struct s_ordine *head;
  struct s_ordine *tail;
} queue_ordini;


ingrediente *tab_ingredienti[MAX_NUM_INGREDIENTI];
ricetta *tab_ricette[DIM_RICETTE];
queue_ordini *ready_queue = NULL;
queue_ordini *waiting_queue = NULL;
queue_ordini* to_remove = NULL;
int pos_array = 0; // dim array dinamico x ordini pronti
int time = 0;
int periodo;
int capienza;

//=====FUNZIONI HASH =====
unsigned int hash(const char *chiave)
{
  // x tab_ingredienti
  unsigned long hs = 5381;
  int c = *chiave++;
  while (c)
  {
    hs = ((hs << 5)+ hs)+c;
    c = *chiave++;
  }
  return hs%MAX_NUM_INGREDIENTI;
}
unsigned int hash2(const char *chiave)
{
  // x tab_ricette
  unsigned long hs = 5381;
  int c = *chiave++;
  while (c)
  {
    hs = ((hs << 5)+ hs)+c;
    c = *chiave++;
  }
  return hs%DIM_RICETTE;
}

//========FUNZIONI========





// aggiunta di un lotto o un ingrediente: quando c'è un rifornimento controllo se esiste già l'ingrediente(collisione 1)
// se esiste e il nome è lo stesso(collisione 1) aggiungo un lotto, altrimenti aggiungo un nuovo ingrediente
// se c'è una collisione con la chiave ma il nome è diverso, aggiungo l'ingrediente alla lista concatenata dal puntatore next.
void rifornimento(char *ing_to_add, char *quantita, char *scadenza)
{
  if(atoi(scadenza) <= time){
    return;
  }
  // printf("Inserendo %s\t%s\t%s\n", ing_to_add, quantita, scadenza);
  unsigned int index = hash(ing_to_add);
  lotto *newLotto = (lotto *)malloc(sizeof(lotto));
  newLotto->scadenza = atoi(scadenza);
  newLotto->quantita = atoi(quantita);
  newLotto->next = NULL;
  ingrediente *curr = tab_ingredienti[index];
  ingrediente *prec = NULL;

  while (curr != NULL)
  {
    if(strcmp(curr->nome,ing_to_add)== 0)
      break;
    prec = curr;
    curr = curr->next;
  }

  if(curr == NULL && prec != NULL) {
    ingrediente * newIng = (ingrediente*)malloc(sizeof(ingrediente));
    newIng->lotti = newLotto;
    newIng->next = NULL;
    strcpy(newIng->nome,ing_to_add);
    
    prec->next = newIng;
    return;
  } else if (curr == NULL && prec == NULL)
  {
    //creo ingrediente
    ingrediente * newIng = (ingrediente*)malloc(sizeof(ingrediente));
    newIng->lotti = newLotto;
    newIng->next = NULL;
    strcpy(newIng->nome,ing_to_add);
    tab_ingredienti[index] = newIng;
    return;
  }
  else if(curr != NULL){
    lotto * tmp = curr->lotti;
    lotto * prv = NULL;

    if(tmp == NULL) {
        curr->lotti = newLotto;
        return;
    }

    while(tmp != NULL && tmp->scadenza <= time) {
      //printf("Elimino lotto scaduto, ingr: %d, scad: %d\n", tmp->quantita, tmp->scadenza);
        prv = tmp->next;
        free(tmp);
        tmp = prv;
    }
    

    //printf("Verifica: %d\n", tmp == curr->lotti);

    lotto* pre = NULL;
    lotto* curr_l = tmp;
    curr->lotti = tmp;

    if(curr_l == NULL) {
        curr->lotti = newLotto;
        return;
    }

    while(curr_l != NULL && curr_l->scadenza <= newLotto->scadenza) {
        pre = curr_l;
        curr_l = curr_l->next;
    }

    if(pre == NULL) {
        newLotto->next = curr_l;
        curr->lotti = newLotto;
    } else {
        pre->next = newLotto;
        newLotto->next = curr_l;
    }  
    
  }

}

// AGGIUNGI INGREDIENTE O CREA RIC. SE NON PRESENTE
void addIngrediente(char *ingrediente, int quantita, char *nome_ricetta, unsigned int index)
{
    ricetta *curr = tab_ricette[index];
    ricetta *prev = NULL;

    // ricerca della ricetta
    while (curr != NULL)
    {
        if (strcmp(curr->nome, nome_ricetta) == 0)
        {
            // trovata ricetta: aggiubngo ingre
            ing_ricetta *temp = curr->ingredienti;
            ing_ricetta *prevIng = NULL;

            // cerco l'ing nella ricette
            while (temp != NULL)
            {
                if (strcmp(temp->nome_ing, ingrediente) == 0)
                {
                    // ing presente, aggiorno quant.
                    temp->quantita += quantita;
                    curr->peso += quantita;
                    return;
                }
                prevIng = temp;
                temp = temp->next;
            }

            // ing non pres: lo aggungo alla fine
            ing_ricetta *newIng = (ing_ricetta *)malloc(sizeof(ing_ricetta));
            strcpy(newIng->nome_ing, ingrediente);
            newIng->quantita = quantita;
            newIng->next = NULL;

            if (prevIng == NULL)
            {
            curr->ingredienti = newIng;
            }
            else
            {
              prevIng->next = newIng;
            }

            curr->peso += quantita;
            return;
        }
        prev = curr;
        curr = curr->next;
    }

    // Ricetta non trovata, ne creo una nuova
    ricetta *newRic = (ricetta *)malloc(sizeof(ricetta));
    strcpy(newRic->nome, nome_ricetta);
    newRic->peso = quantita;
    newRic->next = NULL;

    // Creo il nuovo ingrediente e lo aggiungo alla ricetta
    ing_ricetta *newIng = (ing_ricetta *)malloc(sizeof(ing_ricetta));
    strcpy(newIng->nome_ing, ingrediente);
    newIng->quantita = quantita;
    newIng->next = NULL;

    newRic->ingredienti = newIng;

    // Aggiungo la nuova ricetta alla tabella
    if (prev == NULL)
    {
        tab_ricette[index] = newRic;
    }
    else
    {
        prev->next = newRic;
    }
}

// AGGIUNTA RICETTA
void aggiungi_ricetta()
{

  char nome[255];
  int k = scanf("%s", nome);
  k++;
  //printf("aggiungo %s\n", nome);

  unsigned int index = hash2(nome);
  ricetta *temp = tab_ricette[index];
  while (temp != NULL)
  {
    if (strcmp(temp->nome, nome) == 0)
    {
      printf("ignorato\n");

      char c = getchar();
      while (c != '\n' && c != EOF)
      {
        c = getchar();
      }

      return;
    }
    temp = temp->next;
  }

  

  char ingrediente_r[255];
  int quantita;

  // int peso_count = 0;
  //  Stampa ogni parte della stringa separata da uno spazio
  char c = '!';
  while (c != '\n' && c != EOF && c != (char) 255)
  {

    // NOME INGREDIENTE
    int m = scanf("%s", ingrediente_r);

    m = scanf("%d", &quantita);
    m++;

    addIngrediente(ingrediente_r, quantita, nome, index);

    c = getchar();
  }
  printf("aggiunta\n");
}

// RICERCA PESO DI UN DOLCE
int pesoDolce(char *nome)
{
  unsigned int index = hash2(nome);
  ricetta *curr = tab_ricette[index];
  while (curr != NULL)
  {

    if (strcmp(curr->nome, nome) == 0)
    {
      // trovato
      return curr->peso;
    }
    curr = curr->next;
  }

  return 0; // NON TROVATO
}



// deallocazione struttura ricetta
void deallocRicetta(ricetta *temp)
{
  if (temp != NULL)
  { // ulteriore controllo
    // dealloco ingredienti
    ing_ricetta *ptr = temp->ingredienti;
    ing_ricetta *t = temp->ingredienti;
    while (ptr != NULL)
    {
      t = ptr->next;
      free(ptr);
      ptr = t;
    }
    temp->ingredienti = NULL;
    free(temp);
  }
}


int isOrderPresent(char *nome){
if (waiting_queue == NULL || waiting_queue->head == NULL)
  {
     //continua
  }
  else
  {
    s_ordine *temp = waiting_queue->head;

    if (strcmp(temp->ricetta, nome) == 0)
    {
      // primo elemento
      printf("ordini in sospeso\n");
      return -1;
    }
    while (temp != NULL)
    {
      if (strcmp(temp->ricetta, nome) == 0)
      {
        printf("ordini in sospeso\n");
        return -1;
      }
      temp = temp->next;
    }
  }

if (ready_queue == NULL || ready_queue->head == NULL)
  {
    return 0;
  }
  else
  {
    s_ordine *temp = ready_queue->head;

    if (strcmp(temp->ricetta, nome) == 0)
    {
      // primo elemento
      printf("ordini in sospeso\n");
      return -1;
    }
    while (temp != NULL)
    {
      if (strcmp(temp->ricetta, nome) == 0)
      {
        printf("ordini in sospeso\n");
        return -1;
      }
      temp = temp->next;
    }
  }
return 0;
}


// RIMOZIONE RICETTA
void rimuovi_ricetta(char *nome)
{
  int i = isOrderPresent(nome);
  if (i == -1)
  {
    return;
  }
  

  // se arrivo qui posso togliere la ricetta
  // tolgo eventuali spazi bianchi e fine riga

  unsigned int index = hash2(nome);
  ricetta *curr = tab_ricette[hash2(nome)];
  ricetta *prev = NULL;
  int trovata = 0;
  // cercop la ricetta
  if (curr != NULL && strcmp(curr->nome, nome) == 0)
  {
    trovata = 1; // trovata nel primo elemento(no collisioni)
  }
  while (curr != NULL && curr->next != NULL && trovata == 0)
  {
    prev = curr;
    curr = curr->next;
    if (strcmp(curr->nome, nome) == 0)
    {
      trovata = 1;
      break;
    }
  }
  // se non l'ho trovata, esco
  if (trovata == 0)
  {
    printf("non presente\n");
    return;
  }
  // adesso procedo a eliminarla
  if (prev != NULL && curr != NULL && curr->next != NULL)
  {
    // l'elemento era in mezzo alla lista concatenata delle collisioni
    prev->next = curr->next;
    deallocRicetta(curr);
    printf("rimossa\n");
    return;
  }
  if (curr != NULL && curr->next == NULL && prev != NULL)
  {
    // era l'ultimo elemento
    prev->next = NULL;
    deallocRicetta(curr);
    printf("rimossa\n");
    return;
  }
  if (prev == NULL && curr == tab_ricette[index] && curr->next == NULL)
  {
    // primo elemento
    deallocRicetta(curr);
    tab_ricette[index] = NULL;
    printf("rimossa\n");
    return;
  }
  if (prev == NULL && curr == tab_ricette[index] && curr->next != NULL)
  {
    // primo elemento
    tab_ricette[index] = curr->next;
    deallocRicetta(curr);
    printf("rimossa\n");
    return;
  }
}


// sottrai da tab ing quantita.
int disponib_ingrediente(char *ing, int quantita)
{
  unsigned int index = hash(ing);
  ingrediente* curr = tab_ingredienti[index];


  //1- cerco l'ingrediente nel magazzino ingredienti e vedo se esiste.Se no ritorno ATTESA
  while (curr != NULL)
  {
    if(strcmp(ing, curr->nome) == 0)
      break;
    
    curr = curr->next;
  }
  
  if(curr == NULL)
    return ATTESA;
  
  //se arrivo qui ho trovato l'ingrediente
  //2- inizio a scorrere tra i lotti per vedere se ho la disponibilità richiesta
  int disp = 0;
  lotto * temp = curr->lotti; // testa della lista
  lotto * prev = NULL;
  while (temp != NULL)
  {
    if(temp->scadenza > time && temp->quantita > 0){
      //lotto ok: sommo la quantita perche non scaduto
      disp+=temp->quantita;
      prev = temp;
      temp = temp->next;
      }
    else {
      //lotto scaduto: devo eliminarlo per fare meno scorrimenti possibili
      if (prev == NULL)
      {
        lotto * t;
        if(temp->next != NULL)
          t = temp->next;
        else
          t = NULL;
        free(temp);
        temp = t;
        curr->lotti = temp;
      }
      else
      {
        lotto * t;
        if(temp->next != NULL)
          t = temp->next;
        else
          t = NULL;
        free(temp);
        temp = t;
        prev->next = temp;
      }
   
      if(disp>=quantita)
        return PRONTO;
    
    }
  }
  if(disp>=quantita)
        return PRONTO;
  return ATTESA;
  

}





void rimuovi_ingrediente(char *ing, int quantita)
{
  // printf("ING:%s,QT:%d\n",ing,quantita);
  unsigned int index = hash(ing);
  ingrediente *curr = tab_ingredienti[index];
  // cerco ingrediente
  while (curr != NULL && strcmp(curr->nome, ing) != 0)
  {
    curr = curr->next;
   }
  // printf("Rimuovo %s %p\n",ing,curr);
  // printf("Rimuovoo %s %s\n",ing,curr->nome);

  if(curr == NULL){
    printf("INGREDIENTE NON TROVATO\n");
  }
  
  lotto* curr_l = curr->lotti;
  lotto* prev = NULL;
  int flag = 0;
  while(curr_l != NULL) {

    if(curr_l->quantita >= quantita) {
      curr_l->quantita -= quantita;
      curr->lotti = curr_l;
      break;
    } else {
      quantita -= curr_l->quantita;
      lotto *n = curr_l->next;
      free(curr_l);
      curr_l = n;
      curr->lotti = curr_l;
    }
    if (curr_l->quantita == 0)
    {
      flag = 1;
      lotto * t;
      if(curr_l->next == NULL)
        t = NULL;
      else
        t = curr_l->next;
        //troppe condizioni -------TO DO:sistema qui-------
      if(prev == NULL && curr_l->next != NULL)
        curr->lotti = curr_l->next;
      else if(prev != NULL && curr_l->next != NULL)
        prev->next = curr_l->next;
      else if(prev != NULL && curr_l->next == NULL)
        prev->next = NULL;
      else if(prev == NULL && curr_l->next == NULL)
        curr->lotti = NULL;
      free(curr_l);
      curr_l = t;
    }
    if(flag == 0)
      prev = curr_l;
    flag = 0;
  }

}

// restituisce 0 se trova l'elemento e rimuove la quantità necessaria per il dolce
// 1 se no.L'ordine dovrà essere messo nella waiting queue.-1 se non trova la ricetta
int controllaIngredienti(char *dolce, int quantita_dolci)
{
  //printf("CERCO RICETTA %s",dolce);
  unsigned int index = hash2(dolce);
  ricetta *curr = tab_ricette[index];
  while (curr != NULL)
  {
    if (curr != NULL && strcmp(curr->nome, dolce) == 0)
    {
      // trovato
      break;
    }
    curr = curr->next;
  }
  //printf("TROVATA ricetta: %s\n",curr->nome);

  if (curr == NULL || strcmp(curr->nome,dolce) != 0)
  {
    return -1; // Ricetta non trovata
  }

  ing_ricetta *ing = curr->ingredienti;
  //ricetta trovata, inizio cercare e vedere se rimuovere gli ingredienti se disponibili

  if (ing == NULL) {
    return ATTESA;
  }

  // controllo il primo elemento (il while di sotto non lo farebbe)

  while (ing != NULL)
  {

    int esito = disponib_ingrediente(ing->nome_ing, ing->quantita * quantita_dolci);//PROBLEMA CON TEMPI
    //printf("IG %s %d ESITO %d \n",ing->nome_ing,ing->quantita,esito);
    if (esito == ATTESA)
    {
      return ATTESA; // qualche ingrediente manca.non posso preparare il dolce
    }
    ing = ing->next;
  }

  ing = curr->ingredienti;
  while (ing != NULL)
  {
    rimuovi_ingrediente(ing->nome_ing, ing->quantita* quantita_dolci);
    ing = ing->next;
  }
  
  return PRONTO;
}

// aggiunta ordini alle 2 code
void enqueueAttesa(s_ordine *ord)
{
  
    if (waiting_queue == NULL)
    {
      // Coda vuota, inizializza la coda
      waiting_queue = (queue_ordini *)malloc(sizeof(queue_ordini));
      waiting_queue->head = ord;
      waiting_queue->tail = ord;
      ord->next = NULL;
      ord->prev = NULL;
    }
    else
    {
      // Aggiungi l'ordine alla fine della coda
      ord->next = NULL;                // Nuovo ordine sarà l'ultimo, quindi il next è NULL
      ord->prev = waiting_queue->tail; // Collegamento al vecchio tail
      if (waiting_queue->tail != NULL)
      {
        waiting_queue->tail->next = ord; // Vecchio tail punta al nuovo ordine
      }
      waiting_queue->tail = ord; // Aggiorna il tail della coda
      if (waiting_queue->head == NULL)
      {
        waiting_queue->head = ord; // Se la coda era vuota, head e tail sono lo stesso elemento
      }
    }
    return;
  
}

void enqueuePronto(s_ordine *ord)
{
  //printf("accodo %s %d \n",ord->ricetta, ord->data);
  if(ready_queue == NULL || ready_queue->head == NULL || ready_queue->tail == NULL){
    //coda nulla, testa = coda
    ready_queue = (queue_ordini *)malloc(sizeof(queue_ordini));
    ready_queue->head = ord;
    ready_queue->tail = ord;
    return;
  }

  if (ord->data <= ready_queue->head->data)
  {
    //inserimento in testa
    ready_queue->head->prev = ord;
    ord->next = ready_queue->head;
    ready_queue->head = ord;
    return;
  }
  
  //ultimo caso: devo scorrere dal primo elemento(che sarà il piu recente)
  s_ordine * temp = ready_queue->head;
  s_ordine * prev = NULL;

  //printf("Scorro per inserire %s %d\n",ord->ricetta,ord->data);
  while (temp != NULL)
  {
    //printf("temp %s %d\n",temp->ricetta,temp->data);
    if (temp->data >= ord->data )
    {
      ord->prev = temp->prev;
      temp->prev->next = ord;
      temp->prev = ord;
      ord->next = temp;
      return;
    }    
   
    prev = temp;
    temp = temp->next;
  }
  //printf("Arrivo a tail, %s %d\n",ord->ricetta,ord->data);

  prev->next = ord;
  ord->prev = prev;
  ready_queue->tail = ord;
  ord->next = NULL;

// //se arrivo qua non ho trovato un elemento a data maggiore
//   ready_queue->tail->next = ord;
//   ord->prev = ready_queue->tail;
//   ready_queue->tail = ord;
//   temp->next = ready_queue->tail;
}





// FUNZIONE PER GESTIONE ORDINI
void ordine()
{
  char dolce[255];
  int quantita;
  int q = scanf("%s %d", dolce, &quantita);
  q++;
  s_ordine *newOrd = (s_ordine *)malloc(sizeof(s_ordine));
  newOrd->data = time;
  strcpy(newOrd->ricetta, dolce);
  newOrd->quantita = quantita;
  newOrd->peso = pesoDolce(dolce);
  newOrd->next = NULL;
  newOrd->prev = NULL;

  // guardo se sono presenti lotti per la preparazione.In caso positivo tolgo la quantita necessaria per la preparazione
  // controllo anche la data di scadenza. time<scadenza

  int esito = controllaIngredienti(dolce, quantita);
  //printf("ESITO ordine %s %d\n",dolce,esito);

  if (esito == -1)
  {
    // ricetta non trovata
    printf("rifiutato\n");
    free(newOrd);
    return;
  }
  if (esito == ATTESA)
  {
    printf("accettato\n");
    enqueueAttesa(newOrd);
    return;
  }
  if (esito == PRONTO)
  {
    printf("accettato\n");
    enqueuePronto(newOrd);
    return;
  }
}

// CONTROLLA SE CI SONO ORDINI CHE POSSONO ESSERE PREPARATI DOPO IL RIFORNIMENTO
void checkQueue()
{
  if (waiting_queue == NULL || waiting_queue->head == NULL)
    return;

  s_ordine *curr = waiting_queue->head;
  s_ordine *temp = NULL;
  int esito = -1;

  while (curr != NULL)
  {
    temp = curr->next;
    esito = controllaIngredienti(curr->ricetta,curr->quantita);
    //printf("ESITO %s %d : %d\n", curr->ricetta, curr->data, esito);
    if (esito == PRONTO)
    {
      if (curr == waiting_queue->head && curr->next == NULL)
      {
        //c'era solo la testa
        curr->next = NULL;
        curr->prev = NULL;
        waiting_queue->head = NULL;
        waiting_queue->tail = NULL;
        enqueuePronto(curr);
        return;
      }
      else if (curr == waiting_queue->head && curr->next!=NULL)
      {
        //era in testa ma aveva un next
        waiting_queue->head = curr->next;
        curr->next->prev = NULL;
        curr->next = NULL;
        curr->prev = NULL;
        enqueuePronto(curr);
      }
      
      else if(curr->next!= NULL && curr->prev != NULL){
        //era in mezzo
        curr->prev->next = curr->next;
        curr->next->prev = curr->prev;
        curr->next = NULL;
        curr->prev = NULL;
        enqueuePronto(curr);
      }
      else if(curr == waiting_queue->tail && curr->prev!=NULL){
        waiting_queue->tail = curr->prev;
        curr->prev->next = NULL;
        curr->next=NULL;
        curr->prev = NULL;
        enqueuePronto(curr);
      }
    
    if(temp == NULL)
      break;
    curr = temp;//prev rimane uguale perche è il precedente dell'el. corrente(post modifica)
    }
    else{
      curr = curr->next;
    }
    
    
  }
  

}



// LETTURA E TRADUZIONE INPUT
void execute_command(char *comando)
{
  //printf("CMD:%s \n", comando);
  /*
  //========COMANDI PER DEBUGGING==============
  if (strcmp(comando, "stampa_queue") == 0)
    printQueue(waiting_queue);
  if (strcmp(comando, "stampa_magazzino") == 0)
    printMagazzino();
  if (strcmp(comando, "stampa_ricette") == 0)
    printRicette();
  if (strcmp(comando, "stampa_pronti") == 0)
    printQueue(ready_queue);
*/
  //=============COMANDO ORDINE===============
  if (strcmp(comando, "ordine") == 0)
  {
    ordine();
  }
  //=============COMANDO AGGIUNTA RICETTA===============
  if (strcmp(comando, "aggiungi_ricetta") == 0)
  {
    aggiungi_ricetta();
  }
  //=============COMANDO AGGIUNTA RICETTA===============
  if (strcmp(comando, "rimuovi_ricetta") == 0)
  {
    char t[255];
    int u = scanf("%s", t);
    u++;
    rimuovi_ricetta(t);
  }

  //=============COMANDO RIFORNIMENTO===============
  if (strcmp(comando, "rifornimento") == 0)
  {
    char quantita[255];
    char scadenza[255];
    char ingrediente[255];

    // eseguo rifornimento: iterazione sulla stringa t
    char c = '!';

    while (c != '\n' && c != EOF && c != (char) 255)
    {

      int a = scanf("%s", ingrediente);
      a = scanf("%s", quantita);
      a = scanf("%s", scadenza);
      a++;
      
      // printf("RIFORNIMENTO DI %s SCAD %s QUANTITA %s\n",ingrediente,scadenza,quantita);
      rifornimento(ingrediente, quantita, scadenza); // effettuo rifornimento di un ingrediente

      c = getchar();
    }
    printf("rifornito\n");
    checkQueue();
  }
}








// rimuovo gli elementi da spedire al passaggio del corriere
void delivery()
{
  if (ready_queue == NULL || ready_queue->head == NULL)
  {
    //nessun ordine pronto
    printf("camioncino vuoto\n");
    return; // coda vuota
  }
  
  //creo una queue e ci aggiungo mano a mano gli ordini pronti,dalla testa.
  //in testa a ready queue c'è l'ordine meno recente
  //in testa a to_remove ci dovra essere l'ordine piu pesante tra gli n ordini scelti per la consegna(scelti fino ad arrivare alla capienza)
  s_ordine *to_remove = ready_queue->head;
  s_ordine * curr = ready_queue->head;
  int load = 0;//carico camioncino 
  //scelgo ordini da rimuovere
  s_ordine * prev = NULL;
  int dim = 0;


  //sposto i primi n ordini dalla ready_queue alla nuova lista
  while (curr != NULL )
  {
    if(load+curr->peso*curr->quantita>capienza){
      //printf("Non aggiunto %s %d\n",curr->ricetta,curr->quantita*curr->peso);
      if(prev == NULL) {
      } else {
        prev->next = NULL;
        curr->prev = NULL;
      }

      break;
    }
        
    load += curr->peso * curr->quantita;
    prev = curr;
    curr = curr->next;
    dim++;

   // printf("Cap: %d, Load: %d\n",capienza,load);
  }

   // printf("Final Cap: %d, Load: %d\n",capienza,load);

  ready_queue->head = curr;
    

    
  for(int i = 0; i < dim; i++) {
      s_ordine* ptr = to_remove;

      int max = -1;
      s_ordine* ord_max = NULL;
      for(int j = 0; j < dim; j++) {
          if(max < ptr->peso * ptr->quantita && ptr->data != 0) {
              max = ptr->quantita * ptr->peso;
              ord_max = ptr;
          }
          ptr = ptr->next;
      }
      
      printf("%d %s %d\n", ord_max->data, ord_max->ricetta, ord_max->quantita);
      ord_max->data = 0;
  }

  s_ordine* ord = to_remove;
  while(ord) {
      s_ordine* next = ord->next;
      free(ord);
      ord = next;
  }

  if(ready_queue->head == NULL) {
      free(ready_queue);
      ready_queue = NULL;
  }
  
}




int main(int argc, char *argv[])
{
  int count = 0;
  // inizializz. tab hash
  for (int i = 0; i < MAX_NUM_INGREDIENTI; i++)
  {
    tab_ingredienti[i] = NULL;
  }
  for (int i = 0; i < DIM_RICETTE; i++)
  {
    tab_ricette[i] = NULL;
  }
  // LETTURA INPUT
  char riga[255];
  if (fgets(riga, sizeof(riga), stdin))
  {
    periodo = atoi(strtok(riga, " "));
    capienza = atoi(strtok(NULL, " "));
  }

  char comando[255];

  while (scanf("%s", comando) != EOF)
  {
    //printf("---------------------------------\nTime: %d ",time + 2);
    if (count == periodo)
    {
      // passaggio corriere
      count = 0;
      delivery();
    }
    execute_command(comando);

    time++;
    count++;
  
  }
 if (count == periodo)
    {
      // passaggio corriere
      count = 0;
      delivery();
    }
  // DEALLOCAZIONE STRUTTURE
  for (int i = 0; i < MAX_NUM_INGREDIENTI; i++)
  {
    ingrediente *curr = tab_ingredienti[i];
    while (curr != NULL)
    {
      lotto *l = curr->lotti;
      while (l != NULL)
      {
        lotto *t = l->next;
        free(l);
        l = t;
      }

      ingrediente *next = curr->next;
      free(curr);
      curr = next;
    }
  }

  for (int i = 0; i < DIM_RICETTE; i++)
  {
    ricetta *curr = tab_ricette[i];
    while (curr != NULL)
    {
      // printf("Ricetta: %s\n", curr->nome);
      ing_ricetta *ptr = curr->ingredienti;
      while (ptr != NULL)
      {
        // printf("--free di Ingrediente: %s\n", ptr->nome_ing);
        // printf("PTR: %p\tNEXT: %p\n", ptr, ptr->next);
        ing_ricetta *temp = ptr->next;
        free(ptr);
        ptr = temp;
      }
      ricetta *temp = curr->next;
      free(curr);
      curr = temp;
    }
  }

  if (waiting_queue != NULL)
  {
    s_ordine *o = waiting_queue->head;
    while (o != NULL)
    {
      s_ordine *n = o->next;
      free(o);
      o = n;
    }
  }
  free(waiting_queue);

  //libero ready queue
  if (ready_queue != NULL)
  {
    s_ordine *o = ready_queue->head;
    while (o != NULL)
    {
      s_ordine *n = o->next;
      free(o);
      o = n;
    }
  }
  free(ready_queue);


  return 0;
}