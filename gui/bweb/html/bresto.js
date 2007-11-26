
//   Bweb - A Bacula web interface
//   Bacula® - The Network Backup Solution
//
//   Copyright (C) 2000-2006 Free Software Foundation Europe e.V.
//
//   The main author of Bweb is Eric Bollengier.
//   The main author of Bacula is Kern Sibbald, with contributions from
//   many others, a complete list can be found in the file AUTHORS.
//
//   This program is Free Software; you can redistribute it and/or
//   modify it under the terms of version two of the GNU General Public
//   License as published by the Free Software Foundation plus additions
//   that are listed in the file LICENSE.
//
//   This program is distributed in the hope that it will be useful, but
//   WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//   General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
//   02110-1301, USA.
//
//   Bacula® is a registered trademark of John Walker.
//   The licensor of Bacula is the Free Software Foundation Europe
//   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zurich,
//   Switzerland, email:ftf@fsfeurope.org.

// render if vol is online/offline
function rd_vol_is_online(val)
{
   return '<img src="/bweb/inflag' + val + '.png">';
}

// TODO: fichier ou rep
function rd_file_or_dir(val)
{
   if (val == 'F') {
      return '<img src="/bweb/A.png">';
   } else {
      return '<img src="/bweb/R.png">';
   }
}

Ext.namespace('Ext.brestore');

Ext.brestore.jobid=0;            // selected jobid
Ext.brestore.jobdate='';         // selected date
Ext.brestore.client='';          // selected client
Ext.brestore.rclient='';         // selected client for resto
Ext.brestore.storage='';         // selected storage for resto
Ext.brestore.path='';            // current path (without user location)
Ext.brestore.root_path='';       // user location

Ext.brestore.option_vosb = false;
Ext.brestore.option_vafv = false;
Ext.brestore.dlglaunch;
Ext.BLANK_IMAGE_URL = '/bweb/ext/resources/images/aero/s.gif';  // 1.1

function get_node_path(node)
{
   var temp='';
   for (var p = node; p; p = p.parentNode) {
       if (p.parentNode) {
          if (p.text == '/') {
             temp = p.text + temp;
          } else {
          temp = p.text + '/' + temp;
          }
       }
   }
   return Ext.brestore.root_path + temp;
}


function init_params(baseParams)
{
   baseParams['client']= Ext.brestore.client;

   if (Ext.brestore.option_vosb) {	   
      baseParams['jobid'] = Ext.brestore.jobid;
   } else {
      baseParams['date'] = Ext.brestore.jobdate;
   }
   return baseParams;
}


function ext_init()
{
//////////////////////////////////////////////////////////////:
    var Tree = Ext.tree;
    var tree_loader = new Ext.tree.TreeLoader({
            baseParams:{}, 
            dataUrl:'/cgi-bin/bweb/bresto.pl'
        });

    var tree = new Ext.tree.TreePanel('div-tree', {
        animate:true, 
        loader: tree_loader,
        enableDD:true,
        enableDragDrop: true,
        containerScroll: true
    });

    // set the root node
    var root = new Ext.tree.AsyncTreeNode({
        text: 'Select a client then a job',
        draggable:false,
        id:'source'
    });
    tree.setRootNode(root);
    Ext.brestore.tree = root;

    // render the tree
    tree.render();
//    root.expand();

    tree.on('click', function(node, event) { 
        Ext.brestore.path = get_node_path(node);
	where_field.setValue(Ext.brestore.path);
        file_store.removeAll();
	file_versions_store.removeAll();
        file_store.load({params:init_params({action: 'list_files',
                                             node:node.id})
                       });
        return true;
    });

    tree.on('beforeload', function(e) {
        file_store.removeAll();
        return true;
    });


////////////////////////////////////////////////////////////////

  var file_store = new Ext.data.Store({
        proxy: new Ext.data.HttpProxy({
            url: '/cgi-bin/bweb/bresto.pl',
            method: 'GET',
            params:{}
        }),

        reader: new Ext.data.ArrayReader({
        }, Ext.data.Record.create([
   {name: 'fileid'    },
   {name: 'filenameid'},
   {name: 'pathid'    },
   {name: 'jobid'     },
   {name: 'name'      },
   {name: 'size',     type: 'int'  },
   {name: 'mtime',    type: 'date', dateFormat: 'Y-m-d h:i:s'}
        ]))
   });

   var cm = new Ext.grid.ColumnModel([{
           id:        'name', // id assigned so we can apply custom css (e.g. .x-grid-col-topic b { color:#333 })
           header:    'File',
           dataIndex: 'name',
           width:     200,
           css:       'white-space:normal;'
        },{
           header:    "Size",
           dataIndex: 'size',
           renderer:  human_size,
           width:     50
        },{
           header:    "Date",
           dataIndex: 'mtime',
	   renderer: Ext.util.Format.dateRenderer('Y-d-m h:i:s'),
           width:     100
        },{
           dataIndex: 'pathid',
           hidden: true
        },{
           dataIndex: 'filenameid',
           hidden: true
        },{
           dataIndex: 'fileid',
           hidden: true
        },{
           dataIndex: 'jobid',
           hidden: true
        }
        ]);

    // by default columns are sortable
   cm.defaultSortable = true;
    // create the grid
   var files_grid = new Ext.grid.Grid('div-files', {
        ds: file_store,
        cm: cm,
        ddGroup : 'TreeDD',
        enableDrag: true,
        enableDragDrop: true,
        selModel: new Ext.grid.RowSelectionModel(),
        loadMask: true,
        autoSizeColumns: true,
        enableColLock:false
        
    });
    // when we reload the view,
    // we clear the file version box
    file_store.on('beforeload', function(e) {
        file_versions_store.removeAll();
        return true;
    });

    // TODO: selection only when using dblclick
    files_grid.selModel.on('rowselect', function(e,i,r) { 
        Ext.brestore.filename = r.json[4];
        file_versions_store.load({params:init_params({action: 'list_versions',
						     vafv: Ext.brestore.option_vafv,
  	                                             pathid: r.json[2],
	                                             filenameid: r.json[1]
                                                     })
                                 });
        return true;
    });
    files_grid.render();

//////////////////////////////////////////////////////////////:

  var file_selection_store = new Ext.data.Store({
        proxy: new Ext.data.MemoryProxy(),

        reader: new Ext.data.ArrayReader({
        }, Ext.data.Record.create([
   {name: 'jobid'     },
   {name: 'fileid'    },
   {name: 'filenameid'},
   {name: 'pathid'    },
   {name: 'name'      },
   {name: 'size',     type: 'int'  },
   {name: 'mtime',    type: 'date', dateFormat: 'Y-m-d h:i:s'}
        ]))
   });

   var file_selection_cm = new Ext.grid.ColumnModel([{
           id:        'name', // id assigned so we can apply custom css (e.g. .x-grid-col-topic b { color:#333 })
           header:    "Name",
           dataIndex: 'name',
           width:     250
        },{
           header:    "JobId",
           width:     50,
           dataIndex: 'jobid'
        },{
           header:    "Size",
           dataIndex: 'size',
           renderer:  human_size,
           width:     50
        },{
           header:    "Date",
           dataIndex: 'mtime',
	   renderer: Ext.util.Format.dateRenderer('Y-d-m h:i'),
           width:     100
        },{
           dataIndex: 'pathid',
	   header: 'PathId'
//           hidden: true
        },{
           dataIndex: 'filenameid',
           hidden: true
        },{
           dataIndex: 'fileid',
	   header: 'FileId'
//           hidden: true
        }
        ]);


    // create the grid
   var file_selection_grid = new Ext.grid.Grid('div-file-selection', {
        cm: file_selection_cm,
        ds: file_selection_store,
        ddGroup : 'TreeDD',
        enableDrag: false,
        enableDrop: true,
        selModel: new Ext.grid.RowSelectionModel(),
        loadMask: true,
        enableColLock:false
    });

    var file_selection_record = Ext.data.Record.create(
      {name: 'jobid'},
      {name: 'fileid'},
      {name: 'filenameid'},
      {name: 'pathid'},
      {name: 'size'},
      {name: 'mtime'}
    );
// data.selections[0].json[]
// data.node.id
// http://extjs.com/forum/showthread.php?t=12582&highlight=drag+drop
    var ddrow = new Ext.dd.DropTarget(file_selection_grid.container, {
        ddGroup : 'TreeDD',
        copy:false,
        notifyDrop : function(dd, e, data){
           var r;
           if (data.selections) {
             if (data.grid.id == 'div-files') {
                 for(var i=0;i<data.selections.length;i++) {
                    r = new file_selection_record({
                      jobid:     data.selections[0].json[3],
                      fileid:    data.selections[i].json[0],
                      filenameid:data.selections[i].json[1],
                      pathid:    data.selections[i].json[2],
                      name: Ext.brestore.path + data.selections[i].json[4],
                      size:      data.selections[i].json[5],
                      mtime:     data.selections[i].json[6]
                    });
                    file_selection_store.add(r)
                 }
             }

             if (data.grid.id == 'div-file-versions') {
                    r = new file_selection_record({
                      jobid:     data.selections[0].json[3],
                      fileid:    data.selections[0].json[0],
                      filenameid:data.selections[0].json[1],
                      pathid:    data.selections[0].json[2],
                      name: Ext.brestore.path + Ext.brestore.filename,
                      size:      data.selections[0].json[7],
                      mtime:     data.selections[0].json[8]     
                    });
                    file_selection_store.add(r)
             }
           }
  
           if (data.node) {
              var path= get_node_path(data.node);
              r = new file_selection_record({
                      jobid:     data.node.attributes.jobid,
                      fileid:    0,
                      filenameid:0,
                      pathid:    data.node.id,
                      name:      path,
                      size:      4096,
                      mtime:     0
              });
              file_selection_store.add(r)
           }
  
           return true;
    }});

   file_selection_grid.on('enddrag', function(dd,e) { 
        alert(e) ; return true;
    });
   file_selection_grid.on('notifyDrop', function(dd,e) { 
        alert(e) ; return true;
    });
   file_selection_grid.render();

///////////////////////////////////////////////////////

  var file_versions_store = new Ext.data.Store({
        proxy: new Ext.data.HttpProxy({
            url: '/cgi-bin/bweb/bresto.pl',
            method: 'GET',
            params:{offset:0, limit:50 }
        }),

        reader: new Ext.data.ArrayReader({
        }, Ext.data.Record.create([
   {name: 'fileid'    },
   {name: 'filenameid'},
   {name: 'pathid'    },
   {name: 'jobid'     },
   {name: 'volume'    },
   {name: 'inchanger' },
   {name: 'md5'       },
   {name: 'size',     type: 'int'  },
   {name: 'mtime',    type: 'date', dateFormat: 'Y-m-d h:i:s'}
        ]))
   });

   var file_versions_cm = new Ext.grid.ColumnModel([{
           id:        'name', // id assigned so we can apply custom css (e.g. .x-grid-col-topic b { color:#333 })
           dataIndex: 'name',
           hidden: true
        },{
           header:    "InChanger",
           dataIndex: 'inchanger',
           width:     60,
           renderer:  rd_vol_is_online
        },{
           header:    "Volume",
           dataIndex: 'volume'
        },{
           header:    "JobId",
           width:     50,
           dataIndex: 'jobid'
        },{
           header:    "Size",
           dataIndex: 'size',
           renderer:  human_size,
           width:     50
        },{
           header:    "Date",
           dataIndex: 'mtime',
	   renderer: Ext.util.Format.dateRenderer('Y-d-m h:i:s'),
           width:     100
        },{
           header:    "MD5",
           dataIndex: 'md5',
           width:     160
        },{
	   header:    "pathid",
           dataIndex: 'pathid',
           hidden: true
        },{
           header:    "filenameid",
           dataIndex: 'filenameid',
           hidden: true
        },{
           header:    "fileid",
           dataIndex: 'fileid',
           hidden: true
        }
   ]);

    // by default columns are sortable
   file_versions_cm.defaultSortable = true;

    // create the grid
   var file_versions_grid = new Ext.grid.Grid('div-file-versions', {
        ds: file_versions_store,
        cm: file_versions_cm,
        ddGroup : 'TreeDD',
        enableDrag: true,
        enableDrop: false,
        selModel: new Ext.grid.RowSelectionModel(),
        loadMask: true,
        enableColLock:false
        
    });

    file_versions_grid.on('rowdblclick', function(e) { 
        alert(e) ; file_versions_store.removeAll(); return true;
    });
    file_versions_grid.render();

//////////////////////////////////////////////////////////////:


    var client_store = new Ext.data.Store({
        proxy: new Ext.data.HttpProxy({
            url: '/cgi-bin/bweb/bresto.pl',
            method: 'GET',
            params:{action:'list_client'}
        }),

        reader: new Ext.data.ArrayReader({
        }, Ext.data.Record.create([
           {name: 'name' }
        ]))
    });

    var client_combo = new Ext.form.ComboBox({
        fieldLabel: 'Clients',
        store: client_store,
        displayField:'name',
        typeAhead: true,
        mode: 'local',
        triggerAction: 'all',
        emptyText:'Select a client...',
        selectOnFocus:true,
        forceSelection: true,
        width:135
    });

    client_combo.on('valid', function(e) { 
        Ext.brestore.client = e.getValue();
        Ext.brestore.jobid=0;
        Ext.brestore.jobdate = '';
        Ext.brestore.root_path='';
        job_combo.clearValue();
        file_store.removeAll();
        file_versions_store.removeAll();
        root.collapse(false, false);
        while(root.firstChild){
            root.removeChild(root.firstChild);
        }
        job_store.load( {params:{action: 'list_job',
                                 client:Ext.brestore.client}});
        return true;
    });

//////////////////////////////////////////////////////////////:

    var job_store = new Ext.data.Store({
        proxy: new Ext.data.HttpProxy({
            url: '/cgi-bin/bweb/bresto.pl',
            method: 'GET',
            params:{offset:0, limit:50 }
        }),

        reader: new Ext.data.ArrayReader({
        }, Ext.data.Record.create([
           {name: 'jobid' },
           {name: 'date'  },
           {name: 'jobname' }
        ]))
    });

    var job_combo = new Ext.form.ComboBox({
        fieldLabel: 'Jobs',
        store: job_store,
        displayField:'jobname',
        typeAhead: true,
        mode: 'local',
        triggerAction: 'all',
        emptyText:'Select a job...',
        selectOnFocus:true,
        forceSelection: true,
        width:350
    });

    job_combo.on('select', function(e,c) {
        Ext.brestore.jobid = c.json[0];
	Ext.brestore.jobdate = c.json[1];
        Ext.brestore.root_path='';
        root.setText("Root");
        tree_loader.baseParams = init_params({init:1, action: 'list_dirs'});
        root.reload();
    });

////////////////////////////////////////////////////////////////

    function sel_option(item, check)
    {
	if (item.id == 'id_vosb') {
	   Ext.brestore.option_vosb = check;
	}
	if (item.id == 'id_vafv') {
	   Ext.brestore.option_vafv = check;
	}
    }

    var menu = new Ext.menu.Menu({
        id: 'div-main-menu',
        items: [ 
	   new Ext.menu.CheckItem({
	        id: 'id_vosb',
                text: 'View only selected backup',
                checked: Ext.brestore.option_vosb,
                checkHandler: sel_option
            }),
	   new Ext.menu.CheckItem({
	        id: 'id_vafv',
                text: 'View all file versions',
                checked: Ext.brestore.option_vafv,
                checkHandler: sel_option
            })
        ]
    });
////////////////////////////////////////////////////////////////:

    // create the primary toolbar
    var tb2 = new Ext.Toolbar('div-tb-sel');

    var where_field = new Ext.form.TextField({
            fieldLabel: 'Location',
            name: 'where',
            width:175,
            allowBlank:false
    });

    var tb = new Ext.Toolbar('div-toolbar', [
	client_combo,
        job_combo,
        '-',
        {
          id: 'tb_home',
//        icon: '/bweb/up.gif',
          text: 'Change location',
          cls:'x-btn-text-icon',
          handler: function() { 
                var where = where_field.getValue();
                Ext.brestore.root_path=where;
                root.setText(where);
                tree_loader.baseParams = init_params({ action:'list_dirs', path: where });
                root.reload();
          }
        },
        where_field,
        '-',
        {
            cls: 'x-btn-text-icon bmenu', // icon and text class
            text:'Options',
            menu: menu  // assign menu by instance
        },
	{
	    icon: '/bweb/mR.png', // icons can also be specified inline
            cls: 'x-btn-icon',
	    title: 'restore',
	    handler: function() { 
	        if (Ext.brestore.dlglaunch) {
		   Ext.brestore.dlglaunch.show();
                   return 0;
		}
		Ext.brestore.dlglaunch = new Ext.LayoutDialog("div-resto-dlg", {
//                        modal:true,
                        width:600,
                        height:400,
                        shadow:true,
                        minWidth:300,
                        minHeight:300,
                        proxyDrag: true,
//                        west: {
//	                        split:true,
//	                        initialSize: 150,
//	                        minSize: 100,
//	                        maxSize: 250,
//	                        titlebar: true,
//	                        collapsible: true,
//	                        animate: true
//	                    },
	                center: {
	                        autoScroll:true,
//	                        tabPosition: 'top',
//	                        closeOnTab: true,
//	                        alwaysShowTabs: true
	                }
                });

    var fs = new Ext.form.Form({
        labelAlign: 'right',
        labelWidth: 80
    });

//    var resto_store = new Ext.data.Store({
//        proxy: new Ext.data.HttpProxy({
//            url: '/cgi-bin/bweb/bresto.pl',
//            method: 'GET',
//            params:{action:'list_resto'}
//        }),
//
//        reader: new Ext.data.ArrayReader({
//        }, Ext.data.Record.create([
//           {name: 'name' }
//        ]))
//    });

    var rclient_combo = new Ext.form.ComboBox({
            value: Ext.brestore.client,
            fieldLabel: 'client',
            hiddenName:'client',
            store: client_store,
            displayField:'name',
            typeAhead: true,
            mode: 'local',
            triggerAction: 'all',
            emptyText:'Select a client...',
            selectOnFocus:true,
            width:190
        });
    var where_text = new Ext.form.TextField({
            fieldLabel: 'Where',
            name: 'where',
            value: '/tmp/bacula-restore',
            width:190
        });
    var storage_store = new Ext.data.Store({
        proxy: new Ext.data.HttpProxy({
            url: '/cgi-bin/bweb/bresto.pl',
            method: 'GET',
            params:{action:'list_storage'}
        }),

        reader: new Ext.data.ArrayReader({
        }, Ext.data.Record.create([
           {name: 'name' }
        ]))
    });
    var storage_combo = new Ext.form.ComboBox({
            fieldLabel: 'storage',
            hiddenName:'storage',
            store: storage_store,
            displayField:'name',
            typeAhead: true,
            mode: 'local',
            triggerAction: 'all',
            emptyText:'Select a storage...',
            selectOnFocus:true,
            width:190
        });
////////////////////////////////////////////////////////////////
  var media_store = new Ext.data.Store({
        proxy: new Ext.data.HttpProxy({
            url: '/cgi-bin/bweb/bresto.pl',
            method: 'GET',
            params:{offset:0, limit:50 }
        }),

        reader: new Ext.data.ArrayReader({
        }, Ext.data.Record.create([
   {name: 'volumename'},
   {name: 'enabled'   },
   {name: 'inchanger' }
        ]))
   });

   var media_cm = new Ext.grid.ColumnModel([{
           header:    "InChanger",
           dataIndex: 'inchanger',
           width:     60,
           renderer:  rd_vol_is_online
        }, {
           header:    "Volume",
           id:        'volumename', 
           dataIndex: 'volumename',
	   width:     140
        }
   ]);

    // create the grid
   var media_grid = new Ext.grid.Grid('div-media', {
        ds: media_store,
        cm: media_cm,
        enableDrag: false,
        enableDrop: false,
        loadMask: true,
	width: 200,
        enableColLock:false        
    });

    media_grid.render();

    var items = file_selection_store.data.items;
    var tab_fileid=new Array();
    var tab_jobid=new Array();
    for(var i=0;i<items.length;i++) {
       if (items[i].data['fileid']) {
          tab_fileid.push(items[i].data['fileid']);
       }
       tab_jobid.push(items[i].data['jobid']);
    }
    var res = tab_fileid.join(",");
    var res2 = tab_jobid.join(",");

    media_store.baseParams = init_params({action: 'get_media', jobid: res2, fileid: res});
    media_store.reload();

////////////////////////////////////////////////////////////////
    fs.fieldset(
        {legend:'Media needed'},
        media_grid
    );
    fs.fieldset(
        {legend:'Restore job'},
        new Ext.form.ComboBox({
            fieldLabel: 'Replace',
            hiddenName:'replace',
            store: new Ext.data.SimpleStore({
   		 fields: ['replace'],
		 data : [['always'],['never'],['if newer']]
	    }),
            displayField:'replace',
            typeAhead: true,
            mode: 'local',
            triggerAction: 'all',
            emptyText:'never',
            selectOnFocus:true,
            width:190
        }),
//
//        new Ext.form.ComboBox({
//            fieldLabel: 'job',
//            hiddenName:'job',
//            store: resto_store,
//            displayField:'name',
//            typeAhead: true,
//            mode: 'local',
//            triggerAction: 'all',
//            emptyText:'Select a job...',
//            selectOnFocus:true,
//            width:190
//        }),

        rclient_combo,
        where_text,
        storage_combo
    );
    storage_store.load({params:{action: 'list_storage'}});
//  resto_store.load({params:{action: 'list_resto'}});
    fs.render('div-resto-form');

//      var f = new Ext.form.BasicForm('div-resto-form', {url: '/bweb/test', method: 'GET',
//	                                                baseParams: {init: 1}
// 						       }
//                                     );

    var launch_restore = function() {
	var items = file_selection_store.data.items;
	var tab_fileid=new Array();
	var tab_dirid=new Array();
	var tab_jobid=new Array();
        for(var i=0;i<items.length;i++) {
		if (items[i].data['fileid']) {
			tab_fileid.push(items[i].data['fileid']);
		} else {
			tab_dirid.push(items[i].data['pathid']);
		}
		tab_jobid.push(items[i].data['jobid']);
	}
	var res = ';fileid=' + tab_fileid.join(";fileid=");
	var res2 = ';dirid=' + tab_dirid.join(";dirid=");
	var res3 = ';jobid=' + tab_jobid.join(";jobid=");

	var res4 = ';client=' + rclient_combo.getValue() + ';storage=' + storage_combo.getValue() + ';where=' + where_text.getValue();

	window.location='/cgi-bin/bweb/bresto.pl?action=restore' + res + res2 + res3 + res4;
    } // end launch_restore

    var dialog = Ext.brestore.dlglaunch;
    dialog.addKeyListener(27, dialog.hide, dialog);
    dialog.addButton('Submit', launch_restore);
    dialog.addButton('Close', dialog.hide, dialog);
    
    var layout = dialog.getLayout();
    layout.beginUpdate();
    layout.add('center', new Ext.ContentPanel('div-resto-form', {
                                autoCreate:true, title: 'Third Tab', closable:true, background:true}));
    layout.endUpdate();
    dialog.show();

    }
  }
 ]);

////////////////////////////////////////////////////////////////

    var layout = new Ext.BorderLayout(document.body, {
        north: {
//            split: true
        },
        south: {
            split: true, initialSize: 300
        },
        east: {
            split: true, initialSize: 550
        },
        west: {
            split: true, initialSize: 300
        },
        center: {
            initialSize: 450
        }        
        
    });

layout.beginUpdate();
  layout.add('north', new Ext.ContentPanel('div-toolbar', {
      fitToFrame: true, autoCreate:true,closable: false 
  }));
  layout.add('south', new Ext.ContentPanel('div-file-selection', {
      toolbar: tb2,resizeEl:'div-file-selection',
      fitToFrame: true, autoCreate:true,closable: false
  }));
  layout.add('east', new Ext.ContentPanel('div-file-versions', {
      fitToFrame: true, autoCreate:true,closable: false
  }));
  layout.add('west', new Ext.ContentPanel('div-tree', {
      autoScroll:true, fitToFrame: true, 
      autoCreate:true,closable: false
  }));
  layout.add('center', new Ext.ContentPanel('div-files', {
      autoScroll:true,autoCreate:true,fitToFrame: true
  }));
layout.endUpdate();     


////////////////////////////////////////////////////////////////

//    job_store.load();
    client_store.load({params:{action: 'list_client'}});
//    file_store.load({params:{offset:0, limit:50}});
//    file_versions_store.load({params:{offset:0, limit:50}});
//    file_selection_store.load();

}
Ext.onReady( ext_init );
