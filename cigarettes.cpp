// Consider a system with three smoker processes and one agent process. Each
// smoker continuously rolls a cigarette and then smokes it. But to roll and
// smoke a cigarette, the smoker needs three ingredients: tobacco, paper, and
// matches. 

// One of the smoker processes has paper, another has tobacco, and the
// third has matches. The agent has an infinite supply of all three materials.
// The agent places two of the ingredients on the table. The smoker who has the
// remaining ingredient then makes and smokes a cigarette, signaling the agent
// on completion. The agent then puts out another two of the three ingredients,
// and the cycle repeats.

