sap.ui.define(['rootui5/eve7/controller/Summary.controller',
               'rootui5/eve7/lib/EveManager',
               "sap/ui/model/json/JSONModel",
               "sap/m/Table",
               "sap/m/Column",
               "sap/m/Text",
	       "sap/m/ColumnListItem",
	       "sap/ui/model/Sorter",
   "rootui5/eve7/controller/Ged.controller"
              ], function(SummaryController, EveManager, JSONModel, Table, Column, Text,ColumnListItem, Sorter, GedController) {
   "use strict";

   return SummaryController.extend("custom.MyNewSummary", {

      onInit: function() {
         SummaryController.prototype.onInit.apply(this, arguments);
         this.expandLevel = 0;
      },

      event: function(lst) {
         SummaryController.prototype.event(lst);
         oTree.expandToLevel(0);
      },

      createModel: function() {
         // this is central method now to create summary model
         // one could select top main element which will be shown in SummaryView

         this.summaryElements = {};

         var src = this.mgr.childs[0].childs[2].childs;
         for (var i = 0; i < src.length; i++) {
            if (src[i].fName == "Collections")
               src = src[i].childs;
         }

         return this.createSummaryModel([], src, "/");
      },

      addCollection: function (evt){
         if (!this.table)
            this.createTable();

         if (!this.popover) {
            this.popover = new sap.m.Popover("popupTable", {title:"Popup TEST"});


	    let sw = new sap.m.SearchField();
	    sw.placeholder="Filter";
	    var pt = this.table;
	    sw.attachSearch(function(oEvent) {
	       var txt = oEvent.getParameter("query");
	       let filter = new sap.ui.model.Filter([new sap.ui.model.Filter("firstName", sap.ui.model.FilterOperator.Contains, txt), new sap.ui.model.Filter("lastName", sap.ui.model.FilterOperator.Contains, txt)],false);
	       pt.getBinding("items").filter(filter, "Applications");
	    });

            this.popover.addContent(sw);
            this.popover.addContent(this.table);

	    // footer
	    var pthis = this;
	    let fa = new sap.m.OverflowToolbar();
	    let b1 = new sap.m.Button({text:"AddCollection"});
	    fa.addContent(b1);
	    b1.attachPress(function(oEvent) {
               var oSelectedItem = pt.getSelectedItems();
	       var item1 = oSelectedItem[0];
	       console.log("SELECT ",item1.getBindingContext().getObject());
	    });

	    let b2 = new sap.m.Button({text:"Close"});
	    fa.addContent(b2);
	    b2.attachPress(function(oEvent) {
	       pthis.popover.close();
	    });
	    this.popover.setFooter(fa);
         }
         this.popover.openBy(this.byId("addCollection"));
      },


      createTable: function() {
	 // create some dummy JSON data
	 var data = {
	    names: [
	       {firstName: "Peter", lastName: "Mueller"},
	       {firstName: "Petra", lastName: "Maier"},
	       {firstName: "Thomas", lastName: "Smith"},
	       {firstName: "John", lastName: "Williams"},
	       {firstName: "Maria", lastName: "Jones"}
	    ]
	 };
	 // create a Model with this data
	 var model = new JSONModel();
	 model.setData(data);


	 // create the UI

	 // create a sap.m.Table control
	 var table = new Table("tableTest",{
	    mode:"SingleSelect",
	    columns: [
	       new Column("lastName", {header: new Text({text: "Last Name"})}),
	       new Column("firstName", {header: new Text({text:"First Name"})})
	    ]
	 });
	 table.setIncludeItemInSelection(true);
         this.table = table;
	 table.bActiveHeaders = true;

	 table.attachEvent("columnPress", function(evt) {
	    console.log("press paramerer", evt.getParameters());
            var col = evt.getParameters().column;
	    var sv = false;

	    // init first time ascend
	    if (col.getSortIndicator() == sap.ui.core.SortOrder.Descend || col.getSortIndicator() == sap.ui.core.SortOrder.None ) {
	       sv = true;
	    }
	    else {
               sv = false;
	    }

	    var oSorter = new Sorter(col.sId, sv);
	    var oItems = this.getBinding("items");
	    oItems.sort(oSorter);

	    var indicator = sv ?  sap.ui.core.SortOrder.Descending :  sap.ui.core.SortOrder.Ascending;
	    col.setSortIndicator(indicator);
	 });


	 // bind the Table items to the data collection
	 table.bindItems({
	    path : "/names",
	    template : new ColumnListItem({
	       cells: [
		  new sap.m.Text({text: "{lastName}"}),
		  new sap.m.Text({text: "{firstName}"})
	       ]
	    })
	 });
	 // set the model to the Table, so it knows which data to use
	 table.setModel(model);

      }
   });
});
