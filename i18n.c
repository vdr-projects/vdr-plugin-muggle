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
        "Search",
        "Suchen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Chercher",           // TODO
        "",                                       // TODO
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Delete collection",
        "Sammlung l�schen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Effacer la collection",                  // TODO
        "",                                       // TODO
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Search Result",
        "Suchergebnis",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "R�sultat de recherche",                  // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Album -> Track",
        "Titel nach Album",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Titre apr�s albume",                     // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Genre -> Year -> Track",
        "Titel nach Genre und Jahr",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Genre -> Ann�e -> Titre",                // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Artist -> Track",
        "Titel nach Interpret",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "interpr�te -> titre",                    // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Genre -> Artist -> Album -> Track",
        "Album nach Genre und Interpret",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Genre -> interpr�te -> album -> Titre",  // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Artist -> Album -> Track",
        "Album nach Interpret",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "interpr�te -> album -> Titre",           // TODO
        "",                                       // TODO
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "Ajouter � '%s'",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Add '%s' to '%s'",
        "'%s' zu '%s' hinzuf�gen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Ajouter '%s' � '%s'",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Add all to '%s'",
        "Alles '%s' hinzuf�gen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Ajouter tout � '%s'",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Remove",
        "Entfernen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Effacer",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Remove from '%s'",
        "Aus '%s' entfernen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Effacer de '%s'",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Remove '%s' from '%s'",
        "'%s' aus '%s' entfernen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Effacer '%s' de '%s'",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Remove all entries from '%s'",
        "Alle Eintr�ge aus '%s' entfernen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Effacer tout de '%s'",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Remove all from '%s'",
        "Alles aus '%s' entfernen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Effacer tout de '%s'",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Genre 1",
        "Genre 1",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Genre 1",                                // TODO
        "",                                       // TODO
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Tree View Selection",
        "Suchschema w�hlen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Choisir le sch�ma de recherche",         // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Title -> Album -> Track",
        "Titel -> Album -> Track",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Titre -> Album -> Pi�ce",                // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Collection -> Item",
        "Sammlung - St�ck",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Collectin -> Pi�ce",                     // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Genre -> Decade -> Artist -> Album -> Track",
        "Genre -> Dekade -> Interpret -> Album -> Track",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Genre -> D�cade -> Interpr�te -> Album -> Pi�ce",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Search",
        "Suche",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Chercher",                               // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
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
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {
        "Select search order",
        "Suchschema w�hlen",
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "Choisir le sch�ma de recherchage",       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
        "",                                       // TODO
    },
    {NULL}
};
