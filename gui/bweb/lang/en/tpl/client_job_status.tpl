<br/>
 <div class='titlediv'>
  <h1 class='newstitle'> 
	Running job <TMPL_VAR JobName> on <TMPL_VAR Client>
  </h1>
 </div>
 <div class='bodydiv'>

<table>
 <tr>
  <td> <b> Job Name: </b> </td> <td> <TMPL_VAR jobname> (<TMPL_VAR jobid>) <td> 
 </tr>
 <tr>
  <td> <b> Processing file: </b> </td> <td> <TMPL_VAR "processing file"> </td>
 </tr>
 <tr>
  <td> <b> Speed: </b> </td> <td> <TMPL_VAR "bytes/sec"> B/s</td>
 </tr>
 <tr>
  <td> <b> Files Examined: </b> </td> <td> <TMPL_VAR "files examined"></td>
 </tr>
 <tr>
  <td> <b> Bytes: </b> </td> <td> <TMPL_VAR bytes></td>
 </tr>
<TMPL_IF last_jobbytes>
 <tr>
  <td> <b> Bytes done </b> </td><td> <div id='progress1'><td>
 </tr>
</TMPL_IF>
<TMPL_IF last_jobfiles>
 <tr>
  <td> <b> Files done </b> </td><td> <div id='progress2'><td>
 </tr>
</TMPL_IF>
</table>
<form name='form1' action='?' method='GET'>
<button type="submit" class="bp" name='action' value='dsp_cur_job' 
> <img src='/bweb/update.png' title='Refresh' alt=''>Refresh</button>
<input type='hidden' name='client' value='<TMPL_VAR Client>'>
<input type='hidden' name='jobid' value='<TMPL_VAR JobId>'>
<button type="submit" class="bp" name='action' value='cancel_job'
       onclick="return confirm('Do you want to cancel this job?')"
        title='Cancel job'> <img src='/bweb/cancel.png' alt=''>Cancel</button>
</form>
 </div>

<script type="text/javascript" language="JavaScript">
<TMPL_IF last_jobfiles>
  percent_finish(<TMPL_VAR jobfiles>*100/<TMPL_VAR last_jobfiles>, document.getElementById('progress1'));
</TMPL_IF>
<TMPL_IF last_jobbytes>
  percent_finish(<TMPL_VAR jobbytes>*100/<TMPL_VAR last_jobbytes>, document.getElementById('progress2'));
</TMPL_IF>
  bweb_add_refresh();
</script>

