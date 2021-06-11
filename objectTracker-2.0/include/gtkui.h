#ifndef __GTK_UI_H__
#define __GTK_UI_H__

#include <iostream>
#include <gtk/gtk.h>

namespace gtkui{

class GtkUI {

public :
    GtkUI();
    ~GtkUI();
    
    GtkWindow *window;
    GtkLabel *licenseLab,*psssLab,*resultLab;
    GtkWidget *videoDraw;

    void createUI();
    void showUI();
    // void refreshUI (RefreshData usrData,RefreshObject usrObject);
    void buildSignal(char *signalName);

    bool flags;

private :
    GtkBox *Hbox,*Vbox;
    guint signalID;

    void createController();

};

}

#endif // !__GTK_UI_H__