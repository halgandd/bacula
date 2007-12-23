<br/>
 <div class='titlediv'>
  <h1 class='newstitle'>Assistant d'externalisation de m�dias (partie 2/2)</h1>
 </div>
 <div class='bodydiv'>
  Maintenant, vous devez v�rifier la s�lection et �jecter les m�dias.
   <form action='?' method='get'>
    <table id='compute'></table>
    <table><tr>
    <td style='align: left;'>
    <button type="submit" class="bp" onclick='javascript:window.history.go(-2);' title='Pr�c�dent'> <img src='/bweb/prev.png' alt=''>Pr�c�dent</button>
    </td><td style='align: right;'>
    <button type="submit" class="bp" name='action' value='extern' title='Externaliser la selection'> <img src='/bweb/extern.png' alt=''>Externaliser</button>
   </td></tr>
   </form>
 </div>

<script type="text/javascript" language="JavaScript">

var header = new Array("Nom de volume","Vol Statut",
                       "Type","Nom du Pool","Derni�re �criture", 
                       "Expiration", "S�lection");

var data = new Array();
var chkbox;

<TMPL_LOOP media>
chkbox = document.createElement('INPUT');
chkbox.type  = 'checkbox';
chkbox.value = '<TMPL_VAR volumename>';
chkbox.name  = 'media';
chkbox.checked = 'on';

data.push( new Array(
"<TMPL_VAR volumename>",
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
 table_name:     "compute",
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
 page_nav: true,
 padding: 3,
// disable_sorting: new Array(5,6)
 rows_per_page: rows_per_page
}
);
</script>
