<br/>
 <div class='titlediv'>
  <h1 class='newstitle'>Ayuda para cargar medios (part 2/2)</h1>
 </div>
 <div class='bodydiv'>
  Ahora puede verificar la selecci�n y cargar el medio cargar selecci�n
   <form action='?' method='get'>
    <table id='compute'></table>
    <table><tr>
    <td style='align: left;'>
    <button type="submit" class="bp" onclick='javascript:window.history.go(-2);' title='Volver'> <img src='/bweb/prev.png' alt=''>Volver</button>
    </td><td style='align: right;'>
    <input type="hidden" name='enabled' value='yes'>
    <button type="submit" class="bp" name='action' value='move_media'> 
     <img src='/bweb/intern.png' alt=''></button>
   </td></tr>
   </form>
 </div>

<script type="text/javascript" language="JavaScript">

var header = new Array("Nombre Volumen","Vol Status",
                       "Tipo Medio","Pool Name","Fecha Escritura", 
                       "When expire ?", "Select");

var data = new Array();
var chkbox;

<TMPL_LOOP NAME=media>
chkbox = document.createElement('INPUT');
chkbox.type  = 'checkbox';
chkbox.name = 'media';
chkbox.value= '<TMPL_VAR volumename>';
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
