<br/>
 <div class='titlediv'>
  <h1 class='newstitle'> Client : <TMPL_VAR NAME=clientname> (<TMPL_VAR NAME=label>)</h1>
 </div>
 <div class='bodydiv'>
<form action='?'
     <table id='id<TMPL_VAR NAME=ID>'></table>
     <img src="bgraph.pl?client=<TMPL_VAR NAME=clientname>;graph=job_duration;age=2592000;width=420;height=200" alt='Not enough data' > &nbsp;
     <img src="bgraph.pl?client=<TMPL_VAR NAME=clientname>;graph=job_rate;age=2592000;width=420;height=200" alt='Not enough data'> &nbsp;
     <img src="bgraph.pl?client=<TMPL_VAR NAME=clientname>;graph=job_size;age=2592000;width=420;height=200" alt='Not enough data'> &nbsp;
<!--	<div class="otherboxtitle">
          Actions &nbsp;
        </div>
        <div class="otherbox">
       <h1>Actions</h1> 
       <input type="image" name='action' value='job' title='Show last job'
        src='/bweb/zoom.png'> &nbsp;
       <input type="image" name='action' value='dsp_cur_job' title='Show current job' src='/bweb/zoom.png'> &nbsp;
       <input type="image" name='action' value='client_stat' title='Client stats' src='/bweb/zoom.png'> &nbsp;
        </div>
-->
</form>
 </div>

<script type="text/javascript" language="JavaScript">
var header = new Array("Name", "Nb Jobs", "Nb Bytes", "Nb Files", "Nb Errors");

var data = new Array();

data.push( 
  new Array( "<TMPL_VAR NAME=clientname>", 
	     "<TMPL_VAR NAME=nb_jobs>",
	     "<TMPL_VAR NAME=nb_bytes>",
	     "<TMPL_VAR NAME=nb_files>",
	     "<TMPL_VAR NAME=nb_err>"
             )
) ; 

nrsTable.setup(
{
 table_name:     "id<TMPL_VAR NAME=ID>",
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
 rows_per_page: rows_per_page,
// disable_sorting: new Array(1)
}
);
</script>
