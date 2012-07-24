package chat;


import chat.ChatView;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 *
 * @author Administrador
 */
public class ChatEngine extends Thread{
    ChatView obj;
    
    public ChatEngine(ChatView o){
        super();
        this.obj = o;
    }
    
    @Override
    public void run() {
        while(true){
            if(this.obj.addLine() < 0){
                return;
            }
        }
    }
}
