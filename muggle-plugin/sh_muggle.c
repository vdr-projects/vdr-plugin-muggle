#include <stdio.h>
#include <stdlib.h>
#include "mysql/mysql.h"

#include "content_interface.h"
#include "mgmedia.h"
#include "muggle_tools.h"
#include <unistd.h>

#define DISPLAY_SIZE 20
#define PAGE_JUMP    18


#define EXIT          0
#define TREE_VIEW     1
#define LIST_VIEW     2 
#define PLAYLIST_VIEW 3
#define FILTER_VIEW   4


const char playlist_command_str[] = 
" s : shuffle\n"
" 8 : up\n"
" 2 : down\n"
" 9 : page-up\n"
" 3 : page-down\n"
" p : go to top\n"
" t : switch to tree view\n"
" q : quit";

const char tree_command_str[] = 
" 6 : expand\n"
" 4 : collapse\n"
" 8 : up\n"
" 2 : down\n"
" 9 : page-up\n"
" 3 : page-down\n"
" 5 : add selection to playlist\n"
" p : switch to playlist view\n"
" t : go to root node\n"
" q : quit";

mgSelectionTreeNode* GV_currentNode;
vector<mgSelectionTreeNode*> GV_treeDisplay;

mgPlaylist *GV_currentPlaylist;

void print_tree(mgSelectionTreeNode* node, string tab)
{
    vector<mgSelectionTreeNode*> children;
    vector<mgSelectionTreeNode*>::iterator iter;

    tab = tab + " ";
    printf("%2d %c %s-- Id: '%s', level %d '%s' \n", 
	   GV_treeDisplay.size(), (node->isExpanded()?'x':'-'), tab.c_str(),
	   node->getID().c_str(),  node->getLevel(),
	   node->getLabel().c_str());
//    printf("    %sRestrictions: '%s'\n", tab.c_str(), node->getRestrictions().c_str());
    GV_treeDisplay.push_back(node);
    if(node->isExpanded())
    {
	children =  node->getChildren();
	for(iter = children.begin(); iter != children.end(); iter++)
	{
	    print_tree(*iter, tab);
	}
    }
}
void print_node(mgSelectionTreeNode* node, unsigned int selected, unsigned int start)
{
    vector<mgSelectionTreeNode*> children;
    vector<mgSelectionTreeNode*>::iterator iter;
    mgSelectionTreeNode* child;
    unsigned int i;

    GV_treeDisplay.clear();
    printf("--%s---\n", node->getLabel().c_str());
    children =  node->getChildren();
    // first, add al nodes to the storage
    for(iter = children.begin(); iter != children.end(); iter++)
    {
	GV_treeDisplay.push_back(*iter);
    }
    
    // now display the ones in question
    printf("Displaying %d-%d of %d (%s)\n", start+1, start+DISPLAY_SIZE+1, 
	   GV_treeDisplay.size(), node->getRestrictions().c_str());
    if(start >0)
	printf(".......\n");
    for (i = start; i < start+DISPLAY_SIZE && i< GV_treeDisplay.size(); i++)
    {
	child = GV_treeDisplay[i];
	if(i == selected)
	    printf ("==>");
	else
	    printf("   ");
	printf ("%s\n",child->getLabel().c_str());
    }
    if(i==start+DISPLAY_SIZE)
	printf(".......\n");
}

void print_tracks(vector<mgContentItem*>* tracks, string restriction)
{
    vector<mgContentItem*>::iterator iter;

    printf("======BEGIN_TRACKS=====(restriction:'%s'======\n", restriction.c_str());
    for(iter = tracks->begin(); iter != tracks->end(); iter++)
    {
	printf(" Track: (%d): '%s' \n",(*iter)->getId(), (*iter)->getTitle().c_str());
    }
    printf("======END_TRACKS===========\n");
    printf("press key t continue\n");
    getchar();
}

void PrintPlaylist(mgPlaylist *pl, unsigned int selected, unsigned int start)
{
    vector<mgContentItem*>::iterator iter;
    unsigned int i;
   
    // now display the ones in question
    printf("Playlist '%s' (%d entries)\n", pl->getListname().c_str(),
	   pl->getNumItems()); 
    if(start >0)
	printf(".......\n");
    for (i = start; i<start+DISPLAY_SIZE && i< pl->getNumItems(); i++)
    {
	if(i == selected)
	    printf ("==>");
	else
	    printf("   ");
	printf ("%s\n",pl->getLabel(i, "\t").c_str());
    }
    if(i==start+DISPLAY_SIZE)
	printf(".......\n");
}
int PlaylistView()
{
    char cmd;
    bool loop = true;
    unsigned int selected = 0;
    unsigned int start=0;
 
    while(loop)
    {
	printf("\n\n\n\n\n\n");
	PrintPlaylist(GV_currentPlaylist, selected, start);
	printf("------- enter command ('h' for help) -----------\n");
	cmd = getchar();
	switch (cmd)
	{
	    case 'h': // shuffle  
		printf("%s\n", playlist_command_str);
		break;
	    case 's': // shuffle  
		GV_currentPlaylist->shuffle();
		break;
	    case '8': // up
		if(selected >0) selected--;
		if(selected < start) start = start - PAGE_JUMP;
		break;
	    case '2': // down
		if(selected < (GV_currentPlaylist->getNumItems()-1))
		   selected++;
		if(selected >= start+DISPLAY_SIZE) start = start + PAGE_JUMP;
		break;
	    case '9': // pgup
		if(start >= PAGE_JUMP)
		{
		    start -= PAGE_JUMP;
		    selected -= PAGE_JUMP;
		}
		else
		   mgDebug(6,"Can not go up %d %d", start, selected);
		break;
	    case '3': // pgdown
		mgDebug(6,"pgdown  %d %d", start, selected);
		if(start+ PAGE_JUMP< (GV_currentNode->getChildren()).size())
		{
		    start += PAGE_JUMP;
		    selected += PAGE_JUMP;
		    if(selected >= ((GV_currentNode->getChildren()).size()))
			selected = (GV_currentNode->getChildren()).size();
		}

		break;
	    case 'p': // go to top of playlist view
		 selected=0;
		 start=0;
		 break;
	    case 't': // tree view (on root node)
		cmd = getchar();
		return TREE_VIEW;
		break;
	    case 'q': // tree view (on root node)
		cmd = getchar();
		return EXIT;
		break;
	    default:
		mgWarning("Invalid Command '%c'",cmd);
		
	}
	cmd = getchar();
	
    }// end nodeview loop
    return EXIT;
}
// displays one node (and all its children)
int NodeView()
{
    vector<mgContentItem*>* tracks;
    char cmd;
    bool loop = true;
    unsigned int selected = 0;
    unsigned int start=0;
    while(loop)
    {
	printf("\n\n\n\n\n\n");
	if(!GV_currentNode->isExpanded())
	{
	    GV_currentNode->expand();
	}
	print_node(GV_currentNode,selected, start);
	printf("------- enter command ('h' for help) -----------\n");
	
	cmd = getchar();
	switch (cmd)
	{
	    case 'h': // shuffle  
		printf("%s\n", tree_command_str);
		break;
	    case 'e':
	    case '6':// expand
		GV_currentNode = GV_treeDisplay[selected];
		if(GV_currentNode->expand())
		{
		    selected = 0;
		    start=0;
		}
		else
		{
		    GV_currentNode = GV_currentNode ->getParent();
		}
		break;
	    case 'c' :
	    case '4':
		if(GV_currentNode->getParent() != NULL)
		{
		    GV_currentNode->collapse();
		    GV_currentNode = GV_currentNode->getParent();
		    selected = 0;
		    start=0;
		}
		else
		{
		    mgWarning("Already at top level");
		}
		break;
	    case '8': // up
		if(selected >0) selected--;
		if(selected < start) start = start - PAGE_JUMP;
		break;
	    case '2': // down
		if(selected < ((GV_currentNode->getChildren()).size()-1))
		   selected++;
		if(selected >= start+DISPLAY_SIZE) start = start + PAGE_JUMP;
		break;
	    case '9': // pgup
		if(start >= PAGE_JUMP)
		{
		    start -= PAGE_JUMP;
		    selected -= PAGE_JUMP;
		}
		else
		    mgDebug(6,"Can not go up %d %d", start, selected);
		break;
	    case '3': // pgdown
		mgDebug(6,"pgdown  %d %d", start, selected);
		if(start+ PAGE_JUMP< (GV_currentNode->getChildren()).size())
		{
		    start += PAGE_JUMP;
		    selected += PAGE_JUMP;
		    if(selected >= ((GV_currentNode->getChildren()).size()))
			selected = (GV_currentNode->getChildren()).size();
		}

		break;
	    case '5': // OK ,add to playlist
		tracks = GV_treeDisplay[selected]->getTracks();
		GV_currentPlaylist->appendList(tracks);
		tracks = NULL;
		break;
	    case 'p': // go to playlist view
		cmd = getchar();
		return PLAYLIST_VIEW;
		break;
	    case 't': // tree view (on root node)
		while(GV_currentNode->getParent() != NULL)
		{
		    GV_currentNode->collapse();
		    GV_currentNode = GV_currentNode->getParent();
		}
		selected = 0;
		start=0;
		break;
	    case 'q': // go to playlist view
		cmd = getchar();
		return EXIT;
		break;
	    default:
		mgWarning("Invalid Command '%c'",cmd);
		
	}
	cmd = getchar();
	
    }// end nodeview loop
    return EXIT;
}
int main (int argc, char **argv)
{
    mgMedia* media;
    string tab;
    bool loop = true;
    /* now to connect to the database */
    mgSelectionTreeNode  *root;
    int activeScreen;

    vector<int> listcols;
    mgSetDebugLevel(8);
    media = new mgMedia(mgMedia::GD_MP3);

    mgDebug(3," Staring sh_muggle");
    // create initial tree node in view '1'
    activeScreen = TREE_VIEW;
    root = media->getSelectionRoot();
    root->expand();
    GV_currentNode = root->getChildren()[0];
    GV_currentPlaylist = media->createTemporaryPlaylist();
    listcols.push_back(1);
    listcols.push_back(0);
    GV_currentPlaylist->setDisplayColumns(listcols);
    mgDebug(3," Entering sh_muggle main loop");

    // now switch to the initial view;
    while(loop)
    {
	switch(activeScreen)
	{
	    case TREE_VIEW:
		activeScreen = NodeView();
		break;
	    case PLAYLIST_VIEW:
		activeScreen = PlaylistView();
		break;
	    case EXIT:
	        exit(0);
		break;
	    default:
		mgError("Invalid screen %d");
	}
    }
    mgError("leavingNodeView");

}
