sap.ui.define(['rootui5/eve7/controller/Main.controller',
               'rootui5/eve7/lib/EveManager'
], function(MainController, EveManager) {
   "use strict";    

   return MainController.extend("custom.MyNewMain", {                    

      OnWebsocketClosed : function() {
         var elem = this.byId("centerTitle");
         elem.setHtmlText("<strong style=\"color: red;\">Client Disconnected !</strong>");
      },
      onInit: function() {
         console.log('MAIN CONTROLLER INIT 2');
         MainController.prototype.onInit.apply(this, arguments);
         this.mgr.handle.SetReceiver(this);
	 //this.mgr.
	 console.log("register my controller for init");
         this.mgr.RegisterController(this);
      },

      OnEveManagerInit: function() {
         MainController.prototype.OnEveManagerInit.apply(this, arguments);
         var world = this.mgr.childs[0].childs;

         // this is a prediction that the fireworks GUI is the last element after scenes
         // could loop all the elements in top level and check for typename
         var last = world.length -1;
         console.log("init gui ", last, world);

         if (world[last]._typename == "EventManager") {
            this.fw2gui = (world[last]);

            var pthis = this;
            this.mgr.UT_refresh_event_info = function() {
               console.log("jay ", world[last]);
               pthis.showEventInfo();
            }

             pthis.showEventInfo();
         }
      },
      OnWebsocketMsg : function(handle, msg, offset)
      {
        
         this.mgr.OnWebsocketMsg(handle, msg, offset);
         if ( typeof msg == "string") {
             if(this.refreshTitle)
                this.showEventInfo();
         }
      },
      
      showHelp : function(oEvent) {
         alert("=====User support: dummy@cern.ch");
      },
      
      showEventInfo : function() {
         document.title = this.fw2gui.fName +": " + this.fw2gui.fTitle;
         console.log("document title ", document.title);
         this.byId("runInput").setValue(this.fw2gui.run);
         this.byId("lumiInput").setValue(this.fw2gui.lumi);
         this.byId("eventInput").setValue(this.fw2gui.event);

         this.byId("dateLabel").setText(this.fw2gui.date);
         this.refreshTitle=false;
      },

      nextEvent : function(oEvent) {
         this.refreshTitle=true;
         this.mgr.SendMIR({ "mir":        "NextEvent()",
                            "fElementId": this.fw2gui.fElementId,
                            "class":      "EventManager"
                          });
      },

      prevEvent : function(oEvent) {
         this.refreshTitle=true;
         this.mgr.SendMIR({ "mir":        "PreviousEvent()",
                            "fElementId": this.fw2gui.fElementId,
                            "class":      "EventManager"
                          });
      }

   });
});
