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
 	void MakeCollection();
	vector <const char*> Choices(unsigned int level, unsigned int *current) const;
	bool NeedKey(unsigned int i) const;
	mgParts SelParts(bool distinct, bool deepsort) const;
	bool inCollection(const string Name="") const;
	bool isLanguagelist() const;
	bool isCollectionlist() const;
	bool InitDefaultOrder(unsigned int i=0);
	bool keyIsUnique(mgKeyTypes kt) const { return kt==keyGdUnique;}


    protected:
	bool DeduceKeyValue(mgKeyTypes new_kt,const mgSelection *s,
		vector<mgListItem>& items);
        void InitSelection ();
	const char * const ktName(const mgKeyTypes kt) const;
	mgKeyTypes ktLow() const;
	mgKeyTypes ktHigh() const;
	bool isCollectionOrder() const;

    private:
	void clean();
};

#endif 
