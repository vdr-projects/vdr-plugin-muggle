/*!
 * \file mg_sel_gd.h
 * \brief A general interface to data items, currently only GiantDisc
 *
 * \version $Revision: 1.0 $
 * \date    $Date: 2004-12-07 10:10:35 +0200 (Tue, 07 Dec 2004) $
 * \author  Wolfgang Rohdewald
 * \author  Responsible author: $Author: wr $
 *
 */

#ifndef _MG_SEL_GD_H
#define _MG_SEL_GD_H

#include "mg_selection.h"

using namespace std;

class mgSelectionGd : public mgSelection
{
    public:
        mgSelectionGd(const mgSelection *s);
	mgSelectionGd(const bool fall_through = false);
	vector <const char*> Choices(unsigned int level, unsigned int *current) const;
	mgParts Parts(mgDb *db,bool orderby=true) const;


    protected:
	void DeduceKeyValue(mgKeyTypes new_kt,const mgSelection *s,
		vector<mgListItem>& items);
        void InitSelection ();

    private:
	void clean();
};

#endif 
