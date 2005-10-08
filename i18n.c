/*
 * i18n.c: Internationalization
 *
 * See the README file for copyright information and how to reach the author.
 * Traduction en Fran�ais Patrice Staudt
 *
 * $Id$
 */

#include "i18n.h"

const tI18nPhrase Phrases[] =
{

    {
        "Sort by count",
        "Nach H�ufigkeit sortieren",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Sort by count",       			// TODO
        "",                                       // TODO
        "J�rjest� lukum��r�n mukaan",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Key %d",
        "Schl�sselfeld %d",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Key %d",           			// TODO
        "",                                       // TODO
        "Avain %d",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Create",
        "Neu",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Nouveau",           			// TODO
        "",                                       // TODO
        "Luo",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Browse",
        "Navigieren",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Naviguer",           // TODO
        "",                                       // TODO
        "Selaa",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Order",
        "Sortierung",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Ordre",           // TODO
        "",                                       // TODO
        "J�rjest�",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Collections",
        "Sammlungen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Collections",               // TODO
        "",                                       // TODO
        "Kokoelmat",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Clear the collection?",
        "Sammlung leeren?",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Vider la collection?",               // TODO
        "",                                       // TODO
        "Tyhjennet��nk� kokoelma?",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Delete the collection?",
        "Sammlung l�schen?",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Effacer la collection?",               // TODO
        "",                                       // TODO
        "Tuhotaanko kokoelma?",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Create order",
        "Sortierung neu anlegen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Cr�er un ordre nouveaux",            // TODO
        "",                                       // TODO
        "Luo j�rjestys",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Create collection",
        "Sammlung neu anlegen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Cr�er une nouvelle collection",            // TODO
        "",                                       // TODO
        "Luo kokoelma",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Delete the collection",
        "Sammlung l�schen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Effacer la collection",                  // TODO
        "",                                       // TODO
        "Tuhoa kokoelma",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Delete collection '%s'",
        "Sammlung '%s' l�schen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Effacer la collection '%s'",                  // TODO
        "",                                       // TODO
        "Tuhoa kokoelma '%s'",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Collections",
        "Sammlungen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Collections",                    // TODO
        "",                                       // TODO
        "Kokoelmat",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Commands",
        "Befehle",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Commandes",                    // TODO
        "",                                       // TODO
        "Komennot",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Commands:%s",
        "Befehle:%s",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Commandes:%s",                    // TODO
        "",                                       // TODO
        "Komennot: %s",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Collection",
        "Sammlung",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Collection",                             // TODO
        "",                                       // TODO
        "Kokoelma",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "List",
        "Liste",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Liste",                                  // TODO
        "",                                       // TODO
        "Lista",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Export",
        "Exportieren",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Exporter",                      // TODO
        "",                                       // TODO
        "Vie",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Export track list",
        "St�ckliste exportieren",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Exporter la liste",                      // TODO
        "",                                       // TODO
        "Vie kappalelista",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "External playlist commands",
        "Externe Playlist-Kommandos",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "commande externe playlist",              // TODO
        "",                                       // TODO
        "Ulkoiset soittolistakomennot",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Loop mode off",
        "Endlosmodus aus",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "D�clancher le mode r�p�tition",          // TODO
        "",                                       // TODO
        "Jatkuvasoitto poissa",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Loop mode single",
        "Endlosmodus Einzeltitel",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Mode r�p�tition titre seul",             // TODO
        "",                                       // TODO
        "Jatkuvasoitto kappaleelle",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Loop mode full",
        "Endlosmodus alle",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Mode r�p�tition playlist",               // TODO
        "",                                       // TODO
        "Jatkuvasoitto soittolistalle",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Shuffle mode off",
        "Zufallsmodus aus",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "mode all�atoire d�clench�",              // TODO
        "",                                       // TODO
        "Satunnaissoitto poissa",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Shuffle mode normal",
        "Zufallsmodus normal",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Mode all�atoire normal",                 // TODO
        "",                                       // TODO
        "Satunnaissoitto p��ll�",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Artist",
        "Interpret",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Interpr�te",
        "",                                       // TODO
        "Artisti",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "ArtistABC",
        "InterpretABC",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Interpr�teABC",
        "",                                       // TODO
        "ArtistiABC",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Play all",
        "Spiele alles",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Jouer tout",
        "",                                       // TODO
        "Soita kaikki",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Set",
        "Setzen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "D�finir",
        "",                                       // TODO
        "Aseta",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Instant play",
        "Sofort spielen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Jouer en direct",
        "",                                       // TODO
        "Soita nyt",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Instant play '%s'",
        "'%s' sofort spielen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Jouer '%s' en direct",
        "",                                       // TODO
        "Soita '%s' nyt",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Shuffle mode party",
        "Zufallsmodus Party",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Mode all�atoire f�tes",                  // TODO
        "",                                       // TODO
        "Satunnaissoitto biletykseen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Default",
        "Ziel",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Destinataire", // TODO
        "",                                       // TODO
        "Oletus",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "'%s' to collection",
        "'%s' zu Sammlung",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Ajoute '%s' � une collection",                                       // TODO
        "",                                       // TODO
        "'%s' kokoelmaan",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Set default to collection '%s'",
        "Setze Ziel auf Sammlung '%s'",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Changer destination � la collection '%s'",                                       // TODO
        "",                                       // TODO
        "Aseta kokoelma '%s' oletukseksi",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Default collection now is '%s'",
        "Zielsammlung ist nun '%s'",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "La collection destinataire est maintenant '%s'",                                       // TODO
        "",                                       // TODO
        "Oletuskokoelma on nyt '%s'",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Add",
        "Hinzu",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Ajouter",                                       // TODO
        "",                                       // TODO
        "Lis��",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Add to a collection",
        "Zu einer Sammlung hinzuf�gen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Ajouter � une collection",               // TODO
        "",                                       // TODO
        "Lis�� kokoelmaan",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Add to '%s'",
        "Zu '%s' hinzuf�gen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Ajouter � '%s'",                         // TODO
        "",                                       // TODO
        "Lis�� kokoelmaan '%s'",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Remove",
        "Weg",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Effacer",                                // TODO
        "",                                       // TODO
        "Poista",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Clear",
        "Leeren",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Vider",                                // TODO
        "",                                       // TODO
        "Tyhjenn�",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Clear the collection",
        "Sammlung leeren",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Vider la collection",                    // TODO
        "",                                       // TODO
        "Tyhjenn� kokoelma",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Remove from a collection",
        "Aus einer Sammlung entfernen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Effacer d'une collection",                                       // TODO
        "",                                       // TODO
        "Poista kokoelmasta",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "New collection",
        "Neue Sammlung anlegen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Ajouter une collection",                 // TODO
        "",                                       // TODO
        "Uusi kokoelma",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Remove this collection",
        "Diese Sammlung entfernen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Effacer cette collection",               // TODO
        "",                                       // TODO
        "Poista t�m� kokoelma",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Remove entry from this collection",
        "Eintrag aus dieser Sammlung entfernen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Effacer de cette collection",            // TODO
        "",                                       // TODO
        "Poista kappale kokoelmasta",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Added %s entries",
        "%s Eintr�ge hinzugef�gt",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Ajout� %s pi�ces",                       // TODO
        "",                                       // TODO
        "Lis�tty %s kappaletta",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Removed %s entries",
        "%s Eintr�ge entfernt",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Effac� %s pi�ces",                       // TODO
        "",                                       // TODO
        "Poistettu %s kappaletta",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Removed all entries",
        "Alle Eintr�ge entfernt",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Effac� toutes les pi�ces",               // TODO
        "",                                       // TODO
        "Poistettu kaikki kappaleet",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Now playing",
        "Jetzt wird gespielt",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "En jouant",                              // TODO
        "",                                       // TODO
        "Nyt soitetaan",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Rating",
        "Bewertung",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Arvosana",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Decade",
        "Dekade",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "D�cade",                                 // TODO
        "",                                       // TODO
        "Vuosikymmen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Year",
        "Jahr",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Ann�e",                                  // TODO
        "",                                       // TODO
        "Vuosi",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Album",
        "Album",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Album",                                  // TODO
        "",                                       // TODO
        "Albumi",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Folder1",
        "Ordner1",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Folder1",                                // TODO
        "",                                       // TODO
        "Kansio1",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Folder2",
        "Ordner2",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Folder2",                                // TODO
        "",                                       // TODO
        "Kansio2",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Folder3",
        "Ordner3",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Folder3",                                // TODO
        "",                                       // TODO
        "Kansio3",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Folder4",
        "Ordner4",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Folder4",                                // TODO
        "",                                       // TODO
        "Kansio4",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Genre",
        "Genre",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Genre",                                // TODO
        "",                                       // TODO
        "Tyylilaji",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Genre 2",
        "Genre 2",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Genre 2",                                // TODO
        "",                                       // TODO
        "Tyylilaji 2",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Title",
        "Titel",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Titre",                                  // TODO
        "",                                       // TODO
        "Nimi",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "TitleABC",
        "TitelABC",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "TitreABC",                                  // TODO
        "",                                       // TODO
        "NimiABC",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Track",
        "Track",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Pi�ce",                                  // TODO
        "",                                       // TODO
        "Kappale",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "play",
        "spielen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "jouer",                                 // TODO
        "",                                       // TODO
        "soita",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Collection item",
        "Sammlungseintrag",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Pi�ce de collection",                    // TODO
        "",                                       // TODO
        "Kokoelma",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Collection '%s' NOT deleted",
        "Sammlung '%s' NICHT gel�scht",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Collection '%s' PAS effac�e",            // TODO
        "",                                       // TODO
        "Kokoelmaa '%s' ei tuhottu",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Collection '%s' deleted",
        "Sammlung '%s' gel�scht",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Collection '%s' effac�e",                // TODO
        "",                                       // TODO
        "Kokoelma '%s' tuhottu",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Select an order",
        "Sortierung w�hlen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Choisir un ordre",       // TODO
        "",                                       // TODO
        "Valitse j�rjestys",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Synchronize database",
        "Datenbank synchronisieren",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Synchroniser la base des donn�es",       // TODO
        "",                                       // TODO
        "Tahdista tietokanta",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Create database %s?",
        "Datenbank %s anlegen?",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "G�n�rer la base des donn�es %s?",       // TODO
        "",                                       // TODO
        "Luodaanko tietokanta %s?",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Synchronize database with track files?",
        "Datenbank mit Trackdateien synchronisieren?",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Synchroniser la base des donn�es avec les tracks?",       // TODO
        "",                                       // TODO
        "Tahdistetaanko tietokanta kappaleilla?",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Import items?",
        "Titel importieren?",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Importer les titres?",       // TODO
        "",                                       // TODO
        "Tuodaanko kappaleet?",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Imported %d items...",
        "%d Titel importiert...",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "%d titres import�s...",       // TODO
        "",                                       // TODO
        "Tuotiin %d kappaletta...",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Import done:Imported %d items",
        "Import fertig:%d Titel importiert",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Import finis:%d titres import�s...",       // TODO
        "",                                       // TODO
        "Tuonti valmis: %d kappaletta",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {   
        "Muggle",
        "Muggle",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Muggle",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {   
        "Media juggle plugin for VDR",
        "Media juggle plugin f�r VDR",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Muggle-levyautomaatti",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {   
        "Initial loop mode",
        "Schleifenmodus beim Start",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Jatkuvasoitto oletuksena",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {      
        "Initial shuffle mode",
        "Zufallsmodus beim Start",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Satunnaissoitto oletuksena",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {      
        "on",
        "an",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "p��ll�",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {      
        "off",
        "aus",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "pois",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {      
        "Audio mode",
        "Audio Modus",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "��nimoodi",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {      
        "Round",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "py�ristetty",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {      
        "Dither",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "ditteroitu",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {      
        "Use 48kHz mode only",
        "Nur 48kHz nutzen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "K�yt� vain 48kHz moodia",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {   
        "Normalizer level",
        "Normalizer Level",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Normalisoinnin taso",   
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {   
        "Limiter level",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Rajoittimen taso",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {      
        "Display mode",
        "Anzeigemodus",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "N�ytt�moodi",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {      
        "Background mode",
        "Hintergrundmous",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Taustamoodi",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {      
        "Black",
        "Aus (schwarz)",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "musta",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {   
        "Image",
        "Bilder",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "kuvat",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {   
        "Live",
        "TV-Bild",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "live",   
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {   
        "Image show duration (secs)",
        "Anzeigedauer f�r Bilder",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Kuvien n�ytt�aika (sekuntia)",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {   
        "Image cache directory",
        "Verzeichnis f�r konvertierte Bilder",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "V�limuistihakemisto kuville",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {   
        "Use DVB still picture",
        "DVB still picture nutzen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "K�yt� DVB-pys�ytyskuvaa",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {   
        "Delete stale references",
        "Datenbankeintrag l�schen, wenn Datei fehlt",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Tuhoa vanhentuneet viittaukset",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {   
        "yes",
        "Ja",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "kyll�",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {   
        "no",
        "Nein",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "ei",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Order is undefined",
        "Sortierung ist nicht definiert",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Ordre pas d�fini",       // TODO
        "",                                       // TODO
        "J�rjestyst� ei ole m��ritelty",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "%s not readable, errno=%d",
        "%s nicht lesbar, errno=%d",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Ne peux pas lire %s, errno=%d",
        "",                                       // TODO
        "%s ei luettavissa, virhe=%d",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "%s..%s not readable, errno=%d",
        "%s..%s nicht lesbar, errno=%d",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Ne peux pas lire %s..%s, errno=%d",
        "",                                       // TODO
        "%s..%s ei luettavissa, virhe=%d",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {NULL}
};
