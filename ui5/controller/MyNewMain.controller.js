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
     //    this.mgr.RegisterController(this);
      },

      OnEveManagerInit: function() {

         MainController.prototype.OnEveManagerInit.apply(this, arguments);
         var world = this.mgr.childs[0].childs;

         // this is a prediction that the fireworks GUI is the last element after scenes
         // could loop all the elements in top level and check for typename
         var last = world.length -1;

         if (world[last]._typename == "EventManager") {
            this.fw2gui = (world[last]);
            this.showEventInfo();
         }
      },

      OnWebsocketMsg : function(handle, msg, offset)
      {
         this.mgr.OnWebsocketMsg(handle, msg, offset);
      },


      sceneElementChange: function(msg) {
         console.log("ddddddxxxxx sceneElementChange", msg)
      },

      showHelp : function(oEvent) {
         alert("=====User support: dummy@cern.ch");
      },

      showEventInfo : function() {
         // console.log("showEventInfo");
         let ei = this.fw2gui.fTitle.split("/");
         var event = ei[0];
         var nevents = ei[2];
         var run = ei[2];
         var lumi = ei[3];
         document.title = this.fw2gui.fName +": " + event + "/" + nevents;

         this.byId("runInput").setValue(run);
         this.byId("lumiInput").setValue(lumi);
         this.byId("eventInput").setValue(event);
      },

      nextEvent : function(oEvent) {
          this.mgr.SendMIR("NextEvent()", this.fw2gui.fElementId, "EventManager");
      },

      prevEvent : function(oEvent) {
         this.mgr.SendMIR("PreviousEvent()", this.fw2gui.fElementId, "EventManager");
      },

      toggleGedEditor: function() {
         this.byId("Summary").getController().toggleEditor();
      }

   });
});
