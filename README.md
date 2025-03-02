# Data-Structures-and-Algorithms-Project
A C project aimed at optimization and the use of appropriate data structures, as the program had to be tested with a randomly generated input file containing about 200 000 lines (each line representing a command). The goal was to execute all commands within a maximum time of 14 seconds and a maximum memory usage of 35MiB.

**Project's Specification Document**  

An industrial bakery wants to improve its order management system and has tasked you with developing software to simulate the bakery's operations. The simulation operates in discrete time steps, with each command executed taking one time unit. The simulation starts at time 0 and must consider the following elements:  

- **Ingredients** for the bakery’s products, each identified by a name (a string of characters).  
- **Recipes** offered by the bakery, each identified by a name and consisting of various quantities of ingredients (specified in grams).  
- The **ingredient stock**, which stores the ingredients used in recipes. Stock is replenished with new batches based on a supply schedule. Each batch has a quantity (in grams) and an expiration date (given as the time step from which the batch expires).  
- **Customer orders** are placed for one or more sweets via an online platform or phone. The bakery immediately begins preparing the ordered sweets. The machines used to prepare the sweets are fast enough to assume that an arbitrary number of sweets can be prepared in a single time step. Ingredients for the order are taken from the stock, prioritizing batches that are closer to expiration. If there are insufficient ingredients to complete an order, it is placed on hold. Multiple orders can be on hold. The bakery continues to process new orders and checks if received ingredients allow the completion of held orders. Held orders are processed in the order they arrived.  
- Periodically, the **delivery truck** picks up the ready orders. Orders are chosen in chronological order, and the truck loads orders based on the available capacity. The weight of each sweet is the sum of the weights of all ingredients. Orders are loaded in descending order of weight, with ties broken by order arrival time.  

The simulation ends after the last command is processed.  

The input text file starts with a line containing two integers: the delivery truck’s frequency and its capacity. This is followed by a sequence of commands, each in the following format. All positive or zero integer values can be represented in 32 bits.  

- **add_recipe** `<recipe_name> <ingredient_name> <quantity>...`  
  Adds a recipe to the catalog. If a recipe with the same name already exists, it is ignored.  
  *Expected output:* "added" or "ignored".  
- **remove_recipe** `<recipe_name>`  
  Removes a recipe from the catalog. It has no effect if the recipe is not found or if there are pending orders related to it.  
  *Expected output:* "removed", "pending orders", or "not found".  
- **replenish** `<ingredient_name> <quantity> <expiration_time>...`  
  Replenishes the bakery with a set of ingredient batches. Each batch is defined by an ingredient, a quantity, and an expiration time.  
  *Expected output:* "replenished".  
- **order** `<recipe_name> <quantity>`  
  Places an order for a specific number of sweets of a given recipe.  
  *Expected output:* "accepted" or "rejected" if the recipe does not exist.  

Additionally, the program outputs the orders in the truck as a sequence of triples: `<order_arrival_time> <recipe_name> <quantity>`, ordered by load sequence. The output is printed before handling commands at time `<kn>` (where `<k>` is 1, 2, …). If the truck is empty, it prints the message "empty truck".
