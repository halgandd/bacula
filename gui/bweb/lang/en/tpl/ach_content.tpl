<br/>
 <div class='titlediv'>
  <h1 class='newstitle'> 
Autochanger: <TMPL_VAR Name> (<TMPL_VAR nb_drive> Drives
<TMPL_IF nb_io><TMPL_VAR nb_io> IMPORT/EXPORT</TMPL_IF>)</h1>
 </div>
 <div class='bodydiv'>
   <form action='?' method='get'>
    <input type='hidden' name='ach' value='<TMPL_VAR name>'>
    <TMPL_IF "Update">
    <font color='red'> You must run update slot, Autochanger status is different from bacula slots </font>
    <br/>
    </TMPL_IF>
    <table border='0'>
    <tr>
    <td valign='top'>
    <div class='otherboxtitle'>
     Tools
    </div>
    <div class='otherbox'>
<button type="submit" class="bp" name='action' value='label_barcodes'
 title='run label barcodes'><img src='/bweb/label.png' alt=''>Label</button>
<TMPL_IF nb_io>
<button type="submit" class="bp" name='action' value='eject'
 title='put selected media on i/o'><img src='/bweb/extern.png' alt=''>Eject</button>
<button type="submit" class="bp" name='action' value='clear_io'
 title='Clear I/O'> <img src='/bweb/intern.png' alt=''>Clear I/O</button>
</TMPL_IF>
<button type="submit" class="bp" name='action' value='update_slots'
 title='run update slots'> <img src='/bweb/update.png' alt=''>Update</button>
<br/><br/>
<button type="submit" class="bp" name='action' value='ach_load'
 title='mount drive'> <img src='/bweb/load.png' alt=''>Mount</button>
<button type="submit" class="bp" name='action' value='ach_unload'
 title='umount drive'> <img src='/bweb/unload.png' alt=''>Umount</button>

   </div>
    <td width='200'/>
    <td>
    <b> Drives: </b><br/>
    <table id='id_drive'></table> <br/>
    </td>
    </tr>
    </table>
    <b> Content: </b><br/>
    <table id='id_ach'></table>
   </form>
 </div>

<script type="text/javascript" language="JavaScript">

var header = new Array("Real Slot", "Slot", "Volume Name",
		       "Vol Bytes","Vol Status",
	               "Media Type","Pool Name","Last Written", 
                       "When expire ?", "Select");

var data = new Array();
var chkbox;

<TMPL_LOOP Slots>
chkbox = document.createElement('INPUT');
chkbox.type  = 'checkbox';
chkbox.name = 'slot';
chkbox.value = '<TMPL_VAR realslot>';

data.push( new Array(
"<TMPL_VAR realslot>",
"<TMPL_VAR slot>",
"<TMPL_VAR volumename>",
human_size(<TMPL_VAR volbytes>),
"<TMPL_VAR volstatus>",
"<TMPL_VAR mediatype>",
"<TMPL_VAR name>",
"<TMPL_VAR lastwritten>",
"<TMPL_VAR expire>",
chkbox
 )
);
</TMPL_LOOP>

nrsTable.setup(
{
 table_name:     "id_ach",
 table_header: header,
 table_data: data,
 up_icon: up_icon,
 down_icon: down_icon,
 prev_icon: prev_icon,
 next_icon: next_icon,
 rew_icon:  rew_icon,
 fwd_icon:  fwd_icon,
// natural_compare: false,
 even_cell_color: even_cell_color,
 odd_cell_color: odd_cell_color, 
 header_color: header_color,
// page_nav: true,
// rows_per_page: rows_per_page,
// disable_sorting: new Array(5,6)
 padding: 3
}
);

var header = new Array("Index", "Drive Name", 
		       "Volume Name", "Select");

var data = new Array();
var chkbox;

<TMPL_LOOP Drives>
chkbox = document.createElement('INPUT');
chkbox.type  = 'checkbox';
chkbox.name = 'drive';
chkbox.value = '<TMPL_VAR index>';

data.push( new Array(
"<TMPL_VAR index>",
"<TMPL_VAR name>",
"<TMPL_VAR load>",
chkbox
 )
);
</TMPL_LOOP>

nrsTable.setup(
{
 table_name:     "id_drive",
 table_header: header,
 table_data: data,
 up_icon: up_icon,
 down_icon: down_icon,
 prev_icon: prev_icon,
 next_icon: next_icon,
 rew_icon:  rew_icon,
 fwd_icon:  fwd_icon,
// natural_compare: false,
 even_cell_color: even_cell_color,
 odd_cell_color: odd_cell_color, 
 header_color: header_color,
// page_nav: true,
// rows_per_page: rows_per_page,
// disable_sorting: new Array(5,6),
 padding: 3
}
);

</script>
