/*
 * i18n.c: Internationalization
 *
 * See the README file for copyright information and how to reach the author.
 * Traduction en Français Patrice Staudt
 *
 * $Id$
 */

#include "i18n.h"

const tI18nPhrase Phrases[] = {

  { "items",
    "Einträge",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Notation",// Traduction en Français Patrice Staudt
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Muggle Media Database",
    "Muggle Media Database",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Base de donnée Muggle Media",// TODO Francais
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Tree View Commands",
    "Browser Befehle",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Commande navigateur",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Search Result",
    "Suchergebnis",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Résultat de recherche",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Album -> Track",
    "Titel nach Album",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Titre après albume",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Genre -> Year -> Track",
    "Titel nach Genre und Jahr",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Genre -> Année -> Titre",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Artist -> Track",
    "Titel nach Interpret",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "interprète -> titre",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Genre -> Artist -> Album -> Track",
    "Album nach Genre und Interpret",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Genre -> interprète -> album -> Titre",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Artist -> Album -> Track",
    "Album nach Interpret",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "interprète -> album -> Titre",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Browser",
    "Browser",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Navigateur",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Other Search",
    "Suchmodus",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Mode rechercher",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Query",
    "Suche",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Rechercher",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Album info",
    "Album Details",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Infos Albume",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Edit PL?",
    "Edit PL?",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Édition PL?",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Filter",
    "Filter",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Filtre",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Track info",
    "Titel Details",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Détails trites",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Edit PL",
    "Edit PL",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Édition PL",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Track Search",
    "Titel Suche",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Chercher titre",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Album Search",
    "Album Suche",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Chercher albume",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Playlist Search",
    "Playlist Suche",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Chercher playlist",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "playlist title",
    "Playlist Titel",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Playlist titre",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "playlist author",
    "Playlist Author",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Playlist auteur",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "album artist",
    "Albuminterpret",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Albume interpret",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "album title",
    "Albumtitel",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Albume titre",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "rating",
    "Bewertung",
    "",// TODO
    "",// TODO
    "",// TODO
    "estimation",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "year (to)",
    "Jahr (bis)",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Année (à)",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "year (from)",
    "Jahr (von)",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Année (de)",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "genre",
    "Genre",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Genre",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "artist",
    "Interpret",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "interprète",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "title",
    "Titel",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "titre",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Add",
    "Hinzu",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "ajouter",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Cycle tree",
    "Browser Modus",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Mode Navigateur",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Playlist",
    "Playliste",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Playlist",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Submenu",
    "Untermenü",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "sous menu",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Load",
    "Laden",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "charger",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Save",
    "Speichern",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "sauvegarder",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Clear",
    "Loeschen",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "effacer",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Mainmenu",
    "Hauptmenue",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Menu principal",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "%d tracks sent to current playlist",
    "%d Titel in aktuelle Playlist?",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "%d mettre titre dans la playlist actuel?",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Load playlist",
    "Playliste laden",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Charger playlist",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Save playlist",
    "Playliste speichern",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "sauvegarder playlist",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Rename playlist",
    "Playliste umbenennen",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Playliste umbenennen",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Clear playlist",
    "Playliste leeren",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Playliste vider",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Remove entry from list",
    "Eintrag aus der Playliste entfernen",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Supprimer de la list",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },  
  { "Export playlist",
    "Playliste exportieren",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Exporter la playlist",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "External playlist commands",
    "Externe Playlist-Kommandos",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "commande externe playlist",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Loop mode off",
    "Endlosmodus aus",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Déclancher le mode répétition",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Loop mode single",
    "Endlosmodus Einzeltitel",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Mode répétition titre seul",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Loop mode full",
    "Endlosmodus Playliste",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Mode répétition playlist",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Shuffle mode off",
    "Zufallssmodus aus",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "mode allèatoire déclenché",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Shuffle mode normal",
    "Zufallssmodus normal",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Mode allèatoire normal",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { "Shuffle mode party",
    "Zufallssmodus Party",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Mode allèatoire fêtes",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
  },
  { NULL }
  };
