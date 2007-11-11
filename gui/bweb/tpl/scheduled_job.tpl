<br/>
 <div class='titlediv'>
  <h1 class='newstitle'> <TMPL_IF title><TMPL_VAR title><TMPL_ELSE>Next Jobs </TMPL_IF></h1>
 </div>
 <div class='bodydiv'>
    <form action='<TMPL_VAR cginame>?' method='GET'>
     <table id='id<TMPL_VAR ID>'></table>
     <button type="submit" class="bp" name='action' title='Run now' value='run_job_mod'>
       <img src='/bweb/R.png' alt=''>  Run now </button>
      <button type="submit" class="bp" name='action' title='Disable' value='disable_job'>
       <img src='/bweb/inflag0.png' alt=''> Disable </button>
    </form>
 </div>

<script type="text/javascript" language="JavaScript">

var header = new Array("Scheduled",
                       "Level",
	               "Type",
	               "Priority", 
                       "Name",
                       "Volume",
	               "Select");

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
