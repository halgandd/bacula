<br/>
 <div class='titlediv'>
  <h1 class='newstitle'> Pr�ximos Jobs </h1>
 </div>
 <div class='bodydiv'>
    <form action='?' method='GET'>
     <table id='id<TMPL_VAR ID>'></table>
     <label>
     <input type="image" name='action' value='run_job_mod'
      src='/bweb/R.png' title='Ejecutar Ahora'>  Ejecutar Ahora
     </label>
     <label>
      <input type="image" name='action' value='disable_job'
       src='/bweb/inflag0.png' title='Desactivar'> Desactivar
      </label>
    </form>
 </div>

<script type="text/javascript" language="JavaScript">

var header = new Array("Programado",
                       "Nivel",
	               "Tipo",
	               "Prioridad", 
                       "Nombre",
                       "Volumen",
	               "Selecci�n");

var data = new Array();
var chkbox;

<TMPL_LOOP list>
chkbox = document.createElement('INPUT');
chkbox.type  = 'radio';
chkbox.name = 'job';
chkbox.value = '<TMPL_VAR name>';

data.push( new Array(
"<TMPL_VAR date>",    
"<TMPL_VAR level>",
"<TMPL_VAR type>",     
"<TMPL_VAR priority>",    
"<TMPL_VAR name>",      
"<TMPL_VAR volume>",
chkbox
 )
);
</TMPL_LOOP>

nrsTable.setup(
{
 table_name:     "id<TMPL_VAR ID>",
 table_header: header,
 table_data: data,
 up_icon: up_icon,
 down_icon: down_icon,
 prev_icon: prev_icon,
 next_icon: next_icon,
 rew_icon:  rew_icon,
 fwd_icon:  fwd_icon,
// natural_compare: true,
 even_cell_color: even_cell_color,
 odd_cell_color: odd_cell_color, 
 header_color: header_color,
 page_nav: true,
 rows_per_page: rows_per_page,
// disable_sorting: new Array(6),
 padding: 3
}
);
</script>
