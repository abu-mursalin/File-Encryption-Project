#include <gtk/gtk.h>
#include <string.h>
#include <ctype.h>
#include<stdbool.h>
typedef long long ll;

FILE *fr ,*fw;
char *path1=NULL, *path2=NULL, *Label=NULL;
bool fo=false, fs=false, ft=false;

ll arr[100],cnt = 0;

/*creat a integer array from password*/
void make_array(char *password){
    int j=0;
    for(int i=0;i<strlen(password)-1;i++){
        if(password[i]==' ' && password[i+1]==' ')continue;
        else password[j++]=password[i];
    }
    password[j]='\0';

    ll ans=0,x=10;
    for(int i=0;i<strlen(password);i++){
        if(password[i]==' '){
            arr[cnt++]=ans;
            ans=0;
        }
        else{
            ans*=x;
            ans+=(password[i]-'0');
        }
    }
    arr[cnt]=ans;
}

/*Caesar cipher encryption algorithm*/
void caesar_cipher_encrypt(int key){
    int c;

    while((c = fgetc(fr)) != EOF){
        char ch = c;
        if(isupper(ch))ch = ((ch+key-65) % 26) + 65;
        if(islower(ch))ch = ((ch+key-97) % 26) + 97;
        fputc(ch,fw);
    }

    fclose(fr);
    fclose(fw);
}

/*Caesar cipher decryption algorithm*/
void caesar_cipher_decrypt(int key){
    int c;

    while((c = fgetc(fr)) != EOF){
        char ch = c;
        if(isupper(ch))ch = ((ch-key-65+26) % 26) + 65;
        if(islower(ch))ch = ((ch-key-97+26) % 26) + 97;
        fputc(ch,fw);
    }

    fclose(fr);
    fclose(fw);
}

/*Xor cipher encryption algorithm*/
void xor_cipher_encrypt(int key){
    int c;

    while((c = fgetc(fr)) != EOF){
        char ch = c;
        ch ^= key;
        fputc(ch,fw);
    }

    fclose(fr);
    fclose(fw);
}

/*Xor cipher decryption algorithm*/
void xor_cipher_decrypt(int key){
    int c;

    while((c = fgetc(fr)) != EOF){
        char ch = c;
        ch ^= key;
        fputc(ch,fw);
    }

    fclose(fr);
    fclose(fw);
}

/*do_encrypt_decrypt function that determine which encryption or decryption algorithm will work*/
static void do_encrypt_decrypt(GtkWidget *button, gpointer user_data){
    GtkWidget ** widgets = (GtkWidget **)user_data;

    GtkWidget *combobox = widgets[0];
    GtkWidget *entry = widgets[1];
    GtkWidget *label = widgets[2];

    char *algorithm = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combobox));
    char *password = gtk_editable_get_text(GTK_EDITABLE(entry));

    if(strlen(password) == 0){
        gtk_label_set_text(GTK_LABEL(label),"Sorry , there is something wrong !");
    }

    /*execute this if the process is encryption*/
    else if(strcmp(Label,"Encrypt") == 0){
        if(strcmp(algorithm,"Caesar Cipher") == 0){
            int key = atoi(password);

            if(key>25 || key<1){
                gtk_editable_set_text(GTK_EDITABLE(entry),"");
                gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Password must be between 1 to 25");
                return;
            }

            else  caesar_cipher_encrypt(key);
        }

        else if(strcmp(algorithm,"Xor Cipher") == 0){
            int key = atoi(password);

            if(key>255 || key<1){
                gtk_editable_set_text(GTK_EDITABLE(entry),"");
                gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Password must be between 1 to 255");
                return;
            }

            else xor_cipher_encrypt(key);
        }

        else if(strcmp(algorithm,"Hill Cipher") == 0){
            make_array(password);
        }

        gtk_label_set_text(GTK_LABEL(label),"Encryption Successful !");
        Label = NULL;
    }

    /*execute this if the process is decryption*/
    else if(strcmp(Label,"Decrypt") == 0){
        if(strcmp(algorithm,"Caesar Cipher") == 0){
            int key = atoi(password);

            if(key>255 || key<1){
                gtk_editable_set_text(GTK_EDITABLE(entry),"");
                gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Password must be between 1 to 255");
                return;
            }

            else caesar_cipher_decrypt(key);
        }

        else if(strcmp(algorithm,"Xor Cipher") == 0){
            int key = atoi(password);

            if(key>255 || key<1){
                gtk_editable_set_text(GTK_EDITABLE(entry),"");
                gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Password must be between 1 to 255");
                return;
            }
            else xor_cipher_decrypt(key);
        }

        else if(strcmp(algorithm,"Hill Cipher") == 0){
            
        }

        gtk_label_set_text(GTK_LABEL(label),"Decryption Successful !");
        Label = NULL;
    }
}

/*used to fix if the process is encrypt or decrypt*/
static void on_toggled(GtkWidget *button, gpointer user_data){
    GtkWidget **widgets = (GtkWidget **)user_data;
    GtkWidget *button1 = widgets[0];
    GtkWidget *label = widgets[1];
    GtkWidget *entry = widgets[2];
    GtkWidget *combobox = widgets[3];

    if(gtk_check_button_get_active(GTK_CHECK_BUTTON(button))){
        Label = gtk_check_button_get_label(GTK_CHECK_BUTTON(button));
        gtk_button_set_label(GTK_BUTTON(button1),Label);
    }

    ft = true;

    /*used to determine and set if the program is ready or not to encrypt or decrypt and give permission to enter password*/
    if(ft && fo && fs){
        gtk_label_set_text(GTK_LABEL(label),"Status : Ready");
        gtk_editable_set_editable(GTK_EDITABLE(entry),TRUE);
        char *algorithm = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combobox));

        if(strcmp(algorithm,"Caesar Cipher") == 0){
            gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Enter your password (1 to 25) :");
        }

        else if(strcmp(algorithm,"Xor Cipher") == 0){
            gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Enter your password (1 to 255) :");
        }

        else if(strcmp(algorithm,"Hill Cipher") == 0){
            gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Enter your password (integer value n^2 times separate by space)");
        }

        ft = false;
        fo = false;
        fs = false;
    }
}

/*used to save the file to proper directory and set the path to button*/
static void on_response_save(GtkNativeDialog *native,int response,gpointer user_data){
    if(response == GTK_RESPONSE_ACCEPT){
        GtkWidget **widgets = (GtkWidget **)user_data;
        GtkButton *button = GTK_BUTTON(widgets[0]);
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
        GFile *file = gtk_file_chooser_get_file(chooser);
        path1 = g_file_get_path(file);
        if(path2 != NULL && (strcmp(path1,path2) == 0))fw = fopen(path1,"r+");
        else fw = fopen(path1,"w");
        GtkWidget *label = gtk_label_new(path1);
        gtk_widget_set_halign(label,GTK_ALIGN_START);
        gtk_button_set_child(button,label);
        g_object_unref(file);

        GtkWidget *entry = widgets[4];
        GtkWidget *combobox = widgets[5];

        fs = true;

        /*used to determine and set if the program is ready or not to encrypt or decrypt and give permission to enter password*/
        if(ft && fo && fs){
            gtk_label_set_text(GTK_LABEL(widgets[2]),"Status : Ready");
            gtk_editable_set_editable(GTK_EDITABLE(entry),TRUE);
            char *algorithm = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combobox));

            if(strcmp(algorithm,"Caesar Cipher") == 0){
                gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Enter your password (1 to 25) :");
            }

            else if(strcmp(algorithm,"Xor Cipher") == 0){
                gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Enter your password (1 to 255) :");
            }

            else if(strcmp(algorithm,"Hill Cipher") == 0){
                gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Enter your password (integer value n^2 times separate by space)");
            }

            ft = false;
            fo = false;
            fs = false;
        }

        /*determine if the file is overwritten or not*/
        if((path1 != NULL) && (path2 != NULL) && strcmp(path1,path2) == 0){
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets[3]),TRUE);
        }
    }
}

/*used to open the file from the native dialog and set the path to the button*/
static void on_response_open(GtkNativeDialog *native,int response,gpointer user_data){
    if(response == GTK_RESPONSE_ACCEPT){
        GtkWidget **widgets = (GtkWidget **)user_data;
        GtkButton *button = GTK_BUTTON(widgets[0]);
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
        GFile *file = gtk_file_chooser_get_file(chooser);
        path2 = g_file_get_path(file);
        fr = fopen(path2,"r");
        GtkWidget *label = gtk_label_new(path2);
        gtk_widget_set_halign(label,GTK_ALIGN_START);
        gtk_button_set_child(button,label);
        g_object_unref(file);

        GtkWidget *entry = widgets[4];
        GtkWidget *combobox = widgets[5];

        fo = true;

        /*used to determine and set if the program is ready or not to encrypt or decrypt and give permission to enter password*/
        if(ft && fo && fs){
            gtk_label_set_text(GTK_LABEL(widgets[2]),"Status : Ready");
            gtk_editable_set_editable(GTK_EDITABLE(entry),TRUE);
            char *algorithm = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combobox));

            if(strcmp(algorithm,"Caesar Cipher") == 0){
                gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Enter your password (1 to 25) :");
            }

            else if(strcmp(algorithm,"Xor Cipher") == 0){
                gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Enter your password (1 to 255) :");
            }

            else if(strcmp(algorithm,"Hill Cipher") == 0){
                gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Enter your password (integer value n^2 times separate by space)");
            }

            ft = false;
            fo = false;
            fs = false;
        }
        
        /*determine if the file is overwritten or not*/
        if((path1 != NULL) && (path2 != NULL) && strcmp(path1,path2) == 0){
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets[3]),TRUE);
        }
    }
}

/*used to crate native dialog for searching a file*/
static void browse_clicked_save(GtkWidget *button, gpointer user_data){
    GtkWidget **widgets = (GtkWidget **)user_data;

    GtkFileChooserNative *native2 = gtk_file_chooser_native_new(
        "Select a File",
        GTK_WINDOW(widgets[1]),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Save",
        "_Cancel"
    );

    /*calling on_response_save function to save file*/
    g_signal_connect(native2,"response",G_CALLBACK(on_response_save),widgets);

    /*show the native dialog*/
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(native2));
}

/*used to clear all process and the set default view*/
static void get_clear(GtkWidget *button, gpointer user_data){
    GtkWidget **widgets = (GtkWidget **)user_data;

    GtkButton *button1 = GTK_BUTTON(widgets[0]);
    GtkWidget *entry = widgets[1];
    GtkButton *button2 = GTK_BUTTON(widgets[2]);
    GtkCheckButton *radio1 = GTK_CHECK_BUTTON(widgets[3]);
    GtkCheckButton *radio2 = GTK_CHECK_BUTTON(widgets[4]);
    GtkComboBox *combobox = GTK_COMBO_BOX(widgets[5]);
    GtkWidget *label = widgets[6];
    GtkButton *button3 = GTK_BUTTON(widgets[7]);
    GtkWidget *label1 = widgets[8];
    GtkCheckButton *checkbox = GTK_CHECK_BUTTON(widgets[9]);

    GtkWidget *label2 = gtk_label_new("C:\\Users\\Username\\Document.txt");
    gtk_widget_set_halign(label2,GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(button1),label2);
    GtkWidget *label3 = gtk_label_new("C:\\Users\\Username\\");
    gtk_widget_set_halign(label3,GTK_ALIGN_START);
    gtk_button_set_child(GTK_BUTTON(button2),label3);
    gtk_editable_set_editable(GTK_EDITABLE(entry),TRUE);
    gtk_editable_set_text(GTK_EDITABLE(entry),"");
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Enter password when you are asked");
    gtk_editable_set_editable(GTK_EDITABLE(entry),FALSE);
    gtk_check_button_set_active(radio1,FALSE);
    gtk_check_button_set_active(radio2,FALSE);
    gtk_check_button_set_active(checkbox,FALSE);
    gtk_combo_box_set_active(combobox,0);
    gtk_label_set_text(GTK_LABEL(label),"");
    gtk_button_set_label(button3,"Default");
    gtk_label_set_text(GTK_LABEL(label1),"Status : Unready");

    Label = NULL;
    path1 = NULL;
    path2 = NULL;
    fo = false;
    fs = false;
    ft = false;
}

/*used to create native dialog for searching a file*/
static void browse_clicked_open(GtkWidget *button , gpointer user_data){
    GtkWidget **widgets = (GtkWidget **)user_data;

    GtkFileChooserNative *native1 = gtk_file_chooser_native_new(
        "Select a File",
        GTK_WINDOW(widgets[1]),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Open",
        "_Cancel"
    );

    /*calling on_response_open function to get a file*/
    g_signal_connect(native1,"response",G_CALLBACK(on_response_open),widgets);

    /*show the native dialog*/
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(native1));
}

/*used to change algorithm when status is ready*/
static void on_algorithm(GtkComboBox *combobox, gpointer user_data){
    GtkWidget **widgets = (GtkWidget **)user_data;

    char *label = gtk_label_get_text(GTK_LABEL(widgets[1]));
    GtkWidget *entry = widgets[0];

    if(strcmp(label,"Status : Ready") == 0){
        char *algorithm = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combobox));

        if(strcmp(algorithm,"Caesar Cipher") == 0){
            gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Enter your password (1 to 25) :");
        }

        else if(strcmp(algorithm,"Xor Cipher") == 0){
            gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Enter your password (1 to 255) :");
        }

        else if(strcmp(algorithm,"Hill Cipher") == 0){
            gtk_entry_set_placeholder_text(GTK_ENTRY(entry),"Enter your password (integer value n^2 times separate by space)");
        }
    }
}

/*used to close the window*/
static void close_window(GtkWidget *button, gpointer user_data){
    exit(0);
}

/*Activate function*/
static void activate(GtkApplication *app, gpointer user_data) {

    /*Creating window*/
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "File Encryptor");
    gtk_window_set_default_size(GTK_WINDOW(window), 700, 400);

    /*Creating vertical box that contains horizontal box*/
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_window_set_child(GTK_WINDOW(window),vbox);
    gtk_widget_set_margin_top(vbox, 40);
    gtk_widget_set_margin_bottom(vbox, 40);
    gtk_widget_set_margin_start(vbox, 40);
    gtk_widget_set_margin_end(vbox, 40);

    /*Creating horizontal box that contains a label*/
    GtkWidget *hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);

    /*Creating a label*/
    GtkWidget *label1 = gtk_label_new("Select a file to encrypt/decrypt");
    gtk_box_append(GTK_BOX(hbox1),label1);
    gtk_box_append(GTK_BOX(vbox),hbox1);

    /*Creating a horizontal box that contains a label and two button*/
    GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);

    /*Creating a label*/
    GtkWidget *label2 = gtk_label_new("File :");
    gtk_box_append(GTK_BOX(hbox2),label2);

    /*Creating a button that contains opening file path*/
    GtkWidget *button1 = gtk_button_new();
    gtk_widget_set_margin_start(button1,130);
    GtkWidget *label3 = gtk_label_new("C:\\Users\\Username\\Document.txt");
    gtk_widget_set_halign(label3,GTK_ALIGN_START);
    gtk_widget_set_valign(label3,GTK_ALIGN_CENTER);
    gtk_button_set_child(GTK_BUTTON(button1),label3);
    gtk_widget_set_hexpand(button1,TRUE);
    gtk_box_append(GTK_BOX(hbox2),button1);

    /*Creating a button by which we search a file to open*/
    GtkWidget *button2 = gtk_button_new_with_label("Browse...");
    gtk_box_append(GTK_BOX(hbox2),button2);
    gtk_box_append(GTK_BOX(vbox),hbox2);

    /*Creating a horizontal box that contains a label and two button*/
    GtkWidget *hbox6 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);

    /*Creating a label*/
    GtkWidget *label9 = gtk_label_new("Output Folder :");
    gtk_box_append(GTK_BOX(hbox6),label9);

    /*Creating a button that contains saved file*/
    GtkWidget *button5 = gtk_button_new();
    gtk_widget_set_margin_start(button5,44);
    GtkWidget *label10 = gtk_label_new("C:\\Users\\Username\\");
    gtk_widget_set_halign(label10,GTK_ALIGN_START);
    gtk_widget_set_valign(label10,GTK_ALIGN_CENTER);
    gtk_button_set_child(GTK_BUTTON(button5),label10);
    gtk_widget_set_hexpand(button5,TRUE);
    gtk_box_append(GTK_BOX(hbox6),button5);

    /*Creating a button by which we search for place where file will be saved*/
    GtkWidget *button6 = gtk_button_new_with_label("Browse...");
    gtk_box_append(GTK_BOX(hbox6),button6);
    gtk_box_append(GTK_BOX(vbox),hbox6);

    /*Creating a horizontal box that contains a button*/
    GtkWidget *hbox7 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);

    /*Creating a check button to determine if the input file and output file is same or not*/
    GtkWidget *checkbox = gtk_check_button_new_with_label("Overwrite origanal");
    gtk_widget_set_sensitive((checkbox), FALSE);
    gtk_widget_set_margin_start(checkbox,170);
    gtk_box_append(GTK_BOX(hbox7),checkbox);
    gtk_box_append(GTK_BOX(vbox),hbox7);

    /*Creating a horizontal box that contains a label and a combobox*/
    GtkWidget *hbox5 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);

    /*Creating a label*/
    GtkWidget *label7 = gtk_label_new("Algorithm :");
    gtk_box_append(GTK_BOX(hbox5),label7);

    /*Creating a combobox by which we can select a algorithm for encryption or decryption*/
    GtkWidget *combobox = gtk_combo_box_text_new();
    gtk_widget_set_margin_start(combobox,75);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox),"Select a algorithm");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox),"Caesar Cipher");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox),"Xor Cipher");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox),"Hill Cipher");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox),"AES");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combobox),0);
    gtk_box_append(GTK_BOX(hbox5),combobox);
    gtk_widget_set_hexpand(combobox,TRUE);
    gtk_box_append(GTK_BOX(vbox),hbox5);
    
    /*Creating a horizontal box that contains a label and radio button*/
    GtkWidget *hbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,38);

    /*Creating a label*/
    GtkWidget *label4 = gtk_label_new("Action :");
    gtk_box_append(GTK_BOX(hbox3),label4);

    /*Creating a checker group by which we can mark one*/
    GtkWidget *radio1 = gtk_check_button_new_with_label("Encrypt");
    gtk_widget_set_margin_start(radio1,70);
    gtk_check_button_set_group(GTK_CHECK_BUTTON(radio1),NULL);
    gtk_box_append(GTK_BOX(hbox3),radio1);

    GtkWidget *radio2 = gtk_check_button_new_with_label("Decrypt");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(radio2),GTK_CHECK_BUTTON(radio1));
    gtk_box_append(GTK_BOX(hbox3),radio2);
    gtk_box_append(GTK_BOX(vbox),hbox3);

    /*Creating a horizontal box that contains a label and a button*/
    GtkWidget *hbox4 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);

    /*Creating a label*/
    GtkWidget *label5 = gtk_label_new("Password :");
    gtk_box_append(GTK_BOX(hbox4),label5);

    /*Creating a entry button where we enter password*/
    GtkWidget *entry1 = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry1),"Enter password when you are asked");
    gtk_editable_set_editable(GTK_EDITABLE(entry1),FALSE);
    gtk_widget_set_margin_start(entry1,80);
    gtk_widget_set_hexpand(entry1,TRUE);
    gtk_box_append(GTK_BOX(hbox4),entry1);
    gtk_box_append(GTK_BOX(vbox),hbox4);

    /*Creating a horizontal box that contains a label and three button*/
    GtkWidget *hbox8 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);

    /*Creating a label*/
    GtkWidget *label11 = gtk_label_new("Status : Unready");
    gtk_widget_set_hexpand(label11,TRUE);
    gtk_widget_set_halign(label11,GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(hbox8),label11);

    /*Creating a button that complete encryption or decryprtion when it is clicked*/
    GtkWidget *button7 = gtk_button_new_with_label("Default");
    gtk_box_append(GTK_BOX(hbox8),button7);

    /*Creating a button that clear everything and set default window*/
    GtkWidget *button8 = gtk_button_new_with_label("Clear");
    gtk_box_append(GTK_BOX(hbox8),button8);

    /*Creating a button by which we can close the window*/
    GtkWidget *button9 = gtk_button_new_with_label("Exit");
    gtk_box_append(GTK_BOX(hbox8),button9);
    gtk_box_append(GTK_BOX(vbox),hbox8);

    /*Creating a horizontal box that contains a label*/
    GtkWidget *hbox9 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);

    /*Creating a label that will show is the encryption or decryption successful or not*/
    GtkWidget *label12 = gtk_label_new("");
    gtk_widget_set_hexpand(label12,TRUE);
    gtk_box_append(GTK_BOX(hbox9),label12);
    gtk_box_append(GTK_BOX(vbox),hbox9);

    /*creating a GtkWidget array that will send to browse_clicked_open function*/
    static GtkWidget *widgets[6];
    widgets[0]=button1;
    widgets[1]=window;  
    widgets[2]=label11;
    widgets[3]=checkbox;
    widgets[4]=entry1;
    widgets[5]=combobox;

    /*browse_clicked_open function call for openting a file by browse button*/
    g_signal_connect(button2,"clicked",G_CALLBACK(browse_clicked_open),widgets);

    /*creating a GtkWidget array that will send to browse_clicked_save function*/
    static GtkWidget *widgets1[6];
    widgets1[0]=button5;
    widgets1[1]=window;
    widgets1[2]=label11;
    widgets1[3]=checkbox;
    widgets1[4]=entry1;
    widgets1[5]=combobox;

    /*browse_clicked_save function call for saving a file by browse button*/
    g_signal_connect(button6,"clicked",G_CALLBACK(browse_clicked_save),widgets1);

    /*creating a GtkWidget array that will send to on_toggled function*/
    static GtkWidget *widgets4[4];
    widgets4[0]=button7;
    widgets4[1]=label11;
    widgets4[2]=entry1;
    widgets4[3]=combobox;

    /*calling on_toggled function for selecting encrypt or decrypt*/
    g_signal_connect(radio1,"toggled",G_CALLBACK(on_toggled),widgets4);
    g_signal_connect(radio2,"toggled",G_CALLBACK(on_toggled),widgets4);

    /*creating a GtkWidget array that will send to do_encrypt_decrypt function*/
    static GtkWidget *widgets2[3];
    widgets2[0]=combobox;
    widgets2[1]=entry1;
    widgets2[2]=label12;

    /*calling do_encrypt_decrypt function to complete encrypt or decrypt*/
    g_signal_connect(button7,"clicked",G_CALLBACK(do_encrypt_decrypt),widgets2);

    /*creating a GtkWidget array that will send to get_clear function*/
    static GtkWidget *widgets3[10];
    widgets3[0]=button1;
    widgets3[1]=entry1;
    widgets3[2]=button5;
    widgets3[3]=radio1;
    widgets3[4]=radio2;
    widgets3[5]=combobox;
    widgets3[6]=label12;
    widgets3[7]=button7;
    widgets3[8]=label11;
    widgets3[9]=checkbox;

    /*calling get_clear function to clear all operation and set default window*/
    g_signal_connect(button8,"clicked",G_CALLBACK(get_clear),widgets3);

    /*creating a GtkWidget array that will send to on_algorithm function*/
    static GtkWidget *widgets5[2];
    widgets5[0]=entry1;
    widgets5[1]=label11;

    /*calling on_algorithm function to select a algorithm*/
    g_signal_connect(combobox,"changed",G_CALLBACK(on_algorithm),widgets5);

    /*calling close_window function to close the window*/
    g_signal_connect(button9,"clicked",G_CALLBACK(close_window),NULL);

    /*it show the window*/
    gtk_window_present(GTK_WINDOW(window));
}

/*main function by which start to execute program*/
int main(int argc, char **argv) {

    /*creating GtkApplication object*/
    GtkApplication *app = gtk_application_new("com.example.encryption", G_APPLICATION_DEFAULT_FLAGS);

    /*calling activate function*/
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
