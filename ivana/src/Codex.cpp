#include "Codex.h"

const std::vector<CodexEntry>& GetConceptEntries() {
    static std::vector<CodexEntry> entries = {
        {
            "Balanced Anesthesia",
            "Because no single intravenous or inhaled drug produces hypnosis, amnesia, "
            "analgesia, and immobility without unwanted side effects, modern practice "
            "combines smaller doses of several drug classes -- inhaled agents, "
            "sedative-hypnotics, opioids, and neuromuscular blockers -- rather than "
            "pushing one or two drugs to a large dose. This game's hotbar mirrors that "
            "idea: no single agent covers every threat type well."
        },
        {
            "Why Induction Drugs All 'Wear Off' Similarly",
            "Every induction agent here is lipophilic and partitions preferentially into "
            "highly perfused, lipid-rich tissue -- the brain and spinal cord -- which is "
            "why they all act quickly. A single bolus's effect ends mainly through "
            "redistribution into poorly perfused tissue like muscle and fat, not through "
            "metabolism. That's why drugs with very different metabolic clearance "
            "(propofol vs. thiopental) can still produce a similarly short nap after one dose."
        },
        {
            "Context-Sensitive Half-Time",
            "This is the time needed for a drug's plasma level to fall by 50% after "
            "stopping an infusion, and critically, it depends on how long the infusion "
            "ran. Propofol, etomidate, and ketamine barely change with prolonged "
            "infusion; thiopental and diazepam climb steeply. In this game, that shows "
            "up as how much your resource pool 'drifts' if you spam the same agent "
            "instead of letting it clear."
        },
        {
            "GABA-A Receptor Potentiation",
            "Propofol, barbiturates, benzodiazepines, and etomidate all converge on the "
            "same inhibitory receptor complex, the GABA-A chloride channel, though each "
            "binds a different site and produces a different overall profile. This shared "
            "mechanism is part of why they all cause some degree of sedation, respiratory "
            "depression, and additive interaction with one another."
        },
        {
            "NMDA Receptor Antagonism",
            "Ketamine works through an entirely different mechanism -- blockade of the "
            "NMDA glutamate receptor -- which is the pharmacologic reason it produces "
            "genuine analgesia and a dissociative rather than purely hypnotic state, "
            "unlike every GABA-A-acting drug on this roster."
        },
        {
            "Alpha-2 Agonism",
            "Dexmedetomidine acts on an entirely separate target again: central alpha-2 "
            "adrenergic receptors, particularly in the locus ceruleus, which produces "
            "sedation that resembles natural sleep architecture along with "
            "spinally-mediated analgesia and reduced sympathetic outflow."
        },
        {
            "Redistribution vs. Metabolism",
            "A drug can be metabolized slowly (thiopental) yet still wake a patient up "
            "quickly after one dose, because termination of effect after a single bolus "
            "depends on the drug leaving the brain for other tissues, not on the liver "
            "finishing the job. Repeat dosing changes this picture entirely, since those "
            "other tissues eventually saturate and stop absorbing more drug."
        },
        {
            "Why Emergence Delirium Matters",
            "Ketamine's dissociative emergence reactions (vivid dreams, confusion) are its "
            "single biggest practical limitation, enough that many clinicians co-administer "
            "a benzodiazepine specifically to blunt them. Dexmedetomidine, by contrast, may "
            "reduce emergence delirium rather than cause it, which is part of why it has "
            "become popular in pediatric anesthesia."
        },
    };
    return entries;
}
