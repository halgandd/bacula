 <div class='titlediv'>
  <h1 class='newstitle'>Timing Statistics</h1>
 </div>
 <div class='bodydiv'>
<table border='0' valign='top'>
<td>
<form name='form1' action='?' method='GET'>
	<div class="otherboxtitle" valign='top'>
          Options &nbsp;
        </div>
        <div class="otherbox">
<table border='0'>
<tr>
  <td>
    <h2>Time limits</h2>
     <table>
      <tr><td> Begin: </td> <td><input class='formulaire' type='text' 
           id='iso_begin' name='iso_begin' title='YYYY-MM-DD HH:MM' 
           <TMPL_IF qiso_begin> value=<TMPL_VAR qiso_begin> </TMPL_IF> 
           size='16'>
      </td></tr>
      <tr><td> End: </td><td> <input type='text' class='formulaire' 
	id='iso_end' name='iso_end' title='YYYY-MM-DD HH:MM'
	<TMPL_IF qiso_end> value=<TMPL_VAR qiso_end> </TMPL_IF>
        size='16'>
      </td></tr>
     </table>
  </td>
</tr>
<tr>
  <td>
    <h2>Graph</h2>
      <input type='checkbox' 
             <TMPL_IF qusage>checked</TMPL_IF> 
             name="usage"> Drive usage<br/>
      <input type='checkbox' 
             <TMPL_IF qpoolusage>checked</TMPL_IF> 
             name="poolusage"> Utilisation du pool<br/>
      <input type='checkbox' 
             <TMPL_IF qnojob>checked</TMPL_IF> 
             name="nojob"> Cacher les jobs<br/>
      <input type='checkbox' 
             <TMPL_IF qbypool>checked</TMPL_IF> 
             name="bypool"> Order by Pool<br/>
<TMPL_IF db_client_groups>
<tr>
  <td valign='top'>
    <h2>Groupes</h2>
    <select name='client_group' size='10' class='formulaire' multiple>
<TMPL_LOOP db_client_groups>
	<option id= 'group_<TMPL_VAR name>'><TMPL_VAR name></option>
</TMPL_LOOP>
    </select>
  </td>
</tr>
</TMPL_IF>
<TMPL_IF db_pools>
<tr>
  <td valign='top'>
    <h2>Pools de média</h2>
    <select name='pool' size='10' class='formulaire' multiple>
<TMPL_LOOP db_pools>
	<option id= 'pool_<TMPL_VAR name>'><TMPL_VAR name></option>
</TMPL_LOOP>
    </select>
  </td>
</tr>
</TMPL_IF>
  </td>
</tr>
<!--
  <td valign='top'>
    <h2>Niveau</h2>
    <select name='level' class='formulaire'>
      <option id='level_Any' value='Any'>Tous</option>
      <option id='level_F' value='F'>Full</option>
      <option id='level_D' value='D'>Diff&eacute;rentielle</option>
      <option id='level_I' value='I'>Incr&eacute;mentale</option>
    </select>     
  </td><td valign='top'>
    <h2>Statut</h2>
    <select name='status' class='formulaire'>
      <option id='status_Any' value='Any'>Tous</option>
      <option id='status_T'   value='T'>Ok</option>
      <option id='status_f'   value='f'>Erreur</option>
      <option id='status_A'   value='A'>Annul&eacute;</option>
    </select>   
  </td>
</tr>
<tr>
  <td valign='top'>
    <h2>Période</h2>
    <select name='age' class='formulaire'>
      <option id='age_604800'   value='604800'>Cette semaine</option>
      <option id='age_2678400'  value='2678400'>30 derniers jours</option>
      <option id='age_15552000' value='15552000'>Last 6 months</option>
    </select>     
  </td>
  <td  valign='top'>
    <h2>Taille</h2>
     Largeur : &nbsp;<input class='formulaire' type='text' 
			 name='width' value='<TMPL_VAR width>' size='4'><br/>
     Hauteur :  <input type='text' class='formulaire' 
		name='height' value='<TMPL_VAR height>' size='4'><br/>
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
  <td valign='top'> 
    <h2>Nom du Job</h2>
    <select name='jobname' size='15' class='formulaire' multiple>
<TMPL_LOOP NAME=db_jobnames>
      <option><TMPL_VAR NAME=jobname></option>
</TMPL_LOOP>
    </select>
  </td>
</tr>
-->
<tr>
<td>
  <input type='submit' name='action' value='graph' class='formulaire'> 
</td>
</tr>
</table>
        </div>

</form>
</td>
<td>

 <div class="otherboxtitle">
 Current &nbsp;
 </div>
 <div class="otherbox">
 <img src='<TMPL_VAR result>' alt='Rien n a afficher, essayer avec une autre période'>
 </div>

</td>
</table>
 </div>

<script type="text/javascript" language="JavaScript">

  <TMPL_LOOP qclient_groups>
     document.getElementById('group_' + <TMPL_VAR name>).selected = true;
  </TMPL_LOOP>

  <TMPL_LOOP qpools>
     document.getElementById('pool_' + <TMPL_VAR name>).selected = true;
  </TMPL_LOOP>

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

<TMPL_IF qfilesets>
  for (var i=0; i < document.form1.fileset.length; ++i) {
  <TMPL_LOOP qfilesets>
     if (document.form1.fileset[i].value == <TMPL_VAR name>) {
        document.form1.fileset[i].selected = true;
     }
  </TMPL_LOOP>
  }
</TMPL_IF>

<TMPL_IF qjobnames>
  for (var i=0; i < document.form1.jobname.length; ++i) {
  <TMPL_LOOP qjobnames>
     if (document.form1.jobname[i].value == <TMPL_VAR name>) {
        document.form1.jobname[i].selected = true;
     }
  </TMPL_LOOP>
  }
</TMPL_IF>
</script>

