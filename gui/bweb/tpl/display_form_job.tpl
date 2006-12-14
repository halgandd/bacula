<br/>
<div class="otherboxtitle">
  Filter &nbsp;
</div>
<div class="otherbox">
<form name='form1' action='?' method='GET'>
<table border='0'>
<tr>
  <td valign='top'>
    <h2>Level</h2>
    <select name='level' class='formulaire'>
      <option id='level_Any' value='Any'>Any</option>
      <option id='level_F' value='F'>Full</option>
      <option id='level_D' value='D'>Differential</option>
      <option id='level_I' value='I'>Incremental</option>
    </select>     
  </td>
</tr>
<tr>
 <td valign='top'>
    <h2>Status</h2>
    <select name='status' class='formulaire'>
      <option id='status_Any' value='Any'>Any</option>
      <option id='status_T'   value='T'>Ok</option>
      <option id='status_f'   value='f'>Error</option>
      <option id='status_A'   value='A'>Canceled</option>
    </select>     
  </td>
</tr>
<tr>
 <td valign='top'>
    <h2>Pool</h2>
    <select name='pool' class='formulaire'>
      <option id='pool_all' value=''>All</option>
<TMPL_LOOP NAME=db_pools>
      <option id='pool_<TMPL_VAR name>'><TMPL_VAR name></option>
</TMPL_LOOP>
    </select>
  </td>
</tr>
<tr>
  <td valign='top'>
    <h2>Age</h2>
    <select name='age' class='formulaire'>
      <option id='age_604800'   value='604800'>This week</option>
      <option id='age_2678400'  value='2678400'>Last 30 days</option>
      <option id='age_15552000' value='15552000'>Last 6 month</option>
    </select>     
  </td>
 </tr>
 <tr>
  <td valign='bottom'> 
    <h2>Number of items</h2>
    <input type='text' name='limit' value='<TMPL_VAR NAME=limit>' 
	class='formulaire' size='4'>
  </td>
</tr>
<tr>
  <td valign='top'> 
    <h2>Job Type</h2>
    <select name='jobtype' class='formulaire'>
      <option id='jobtype_any' value='all type'>Any</option>
      <option id='jobtype_B' value='B'>Backup</option>
      <option id='jobtype_R' value='R'>Restore</option>
    </select>
  </td>
</tr>
<tr>
  <td valign='top'> 
    <h2>Clients</h2>
    <select name='client' size='15' class='formulaire' multiple>
<TMPL_LOOP NAME=db_clients>
      <option id='client_<TMPL_VAR clientname>'><TMPL_VAR clientname></option>
</TMPL_LOOP>
    </select>
  </td>
</tr>
<!--
<tr>
  <td valign='top'> 
    <h2>File Set</h2>
    <select name='fileset' size='15' class='formulaire' multiple>
<TMPL_LOOP NAME=db_filesets>
      <option id='client_<TMPL_VAR fileset>'><TMPL_VAR NAME=fileset></option>
</TMPL_LOOP>
    </select>
  </td>
</tr>
-->
</table>
  <input type="image" name='action' value='job' src='/bweb/update.png'>

</form>
</div>
<script type="text/javascript" language="JavaScript">

  <TMPL_LOOP qclients>
     document.getElementById('client_' + <TMPL_VAR name>).selected = true;
  </TMPL_LOOP>

  <TMPL_IF status>
     document.getElementById('status_<TMPL_VAR status>').selected=true;
  </TMPL_IF>

  <TMPL_IF level>
     document.getElementById('level_<TMPL_VAR level>').selected=true;
  </TMPL_IF>

  <TMPL_IF age>
     document.getElementById('age_<TMPL_VAR age>').selected=true;
  </TMPL_IF>

  <TMPL_IF jobtype>
     document.getElementById('jobtype_<TMPL_VAR jobtype>').selected=true;
  </TMPL_IF>

  <TMPL_LOOP qfilesets>
     document.getElementById('fileset_' + <TMPL_VAR name>).selected = true;
  </TMPL_LOOP>

  <TMPL_LOOP qpools>
     document.getElementById('pool_' + <TMPL_VAR name>).selected = true;
  </TMPL_LOOP>

</script>

