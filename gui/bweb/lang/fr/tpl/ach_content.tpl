<br/>
 <div class='titlediv'>
  <h1 class='newstitle'> 
Robotique : <TMPL_VAR Name> (<TMPL_VAR nb_drive> Lecteurs
<TMPL_IF nb_io><TMPL_VAR nb_io> IMPORT/EXPORT</TMPL_IF>)</h1>
 </div>
 <div class='bodydiv'>
   <form action='?' method='get'>
    <input type='hidden' name='ach' value='<TMPL_VAR name>'>
    <TMPL_IF "Update">
    <font color='red'> Vous devez lancer la mise � jour des slots, le contenu de la robotique est diff�rent de ce qui est indiqu� dans la base de bacula. </font>
    <br/>
    </TMPL_IF>
    <table border='0'>
    <tr>
    <td valign='top'>
    <div class='otherboxtitle'>
     Options
    </div>
    <div class='otherbox'>
<button type="submit" class="bp" name='action' value='label_barcodes'
 title='Lab�lisation des m�dias, (label barcodes)'><img src='/bweb/label.png' alt=''>Label</button>
<TMPL_IF nb_io>
<button type="submit" class="bp" name='action' value='eject'
 title='Mettre les m�dias s�lectionn�s dans le guichet'><img src='/bweb/extern.png' alt=''>Externaliser</button>
<button type="submit" class="bp" name='action' value='clear_io'
 title='Vider le guichet'> <img src='/bweb/intern.png' alt=''>Vider le guichet</button>
</TMPL_IF>
<button type="submit" class="bp" name='action' value='update_slots'
 title='Mettre � jour la base bacula, (update slots)'> <img src='/bweb/update.png' alt=''>Scanner</button>
<br/><br/>
<button type="submit" class="bp" name='action' value='ach_load'
 title='Charger un lecteur'> <img src='/bweb/load.png' alt=''>Mount</button>
<button type="submit" class="bp" name='action' value='ach_unload'
 title='D�charger un lecteur'> <img src='/bweb/unload.png' alt=''>Umount</button>

   </div>
    <td width='200'/>
    <td>
    <b> Lecteurs : </b><br/>
    <table id='id_drive'></table> <br/>
    </td>
    </tr>
    </table>
    <b> Contenu : </b><br/>
    <table id='id_ach'></table>
   </form>
 </div>

<script type="text/javascript" language="JavaScript">

var header = new Array("Slot r�el", "Slot", "Nom de volume",
		       "Taille","Vol Statut",
	               "Type","Nom du Pool","Derni�re �criture", 
                       "Expiration", "S�lection");

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

var header = new Array("Index", "Lecteurs", 
		       "Nom de volume", "S�lection");

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
