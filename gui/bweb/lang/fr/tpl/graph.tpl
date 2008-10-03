<br/>
 <div class='titlediv'>
  <h1 class='newstitle'>Statistiques</h1>
 </div>
 <div class='bodydiv'>
<table border='0'>
<td>
<form name='form1' action='?' method='GET'>
        <div class="otherboxtitle">
          Options &nbsp;
        </div>
        <div class="otherbox">
<table border='0'>
<tr>
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
      <option id='status_T'   value='T'>Ok</option>
      <option id='status_f'   value='f'>Erreur</option>
      <option id='status_A'   value='A'>Annul&eacute;</option>
      <option id='status_Any' value='Any'>Tous</option>
    </select>   
  </td>
</tr>
<tr>
  <td valign='top'>
    <h2>P�riode</h2>
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
<TMPL_LOOP db_clients>
      <option id='client_<TMPL_VAR clientname>'><TMPL_VAR clientname></option>
</TMPL_LOOP>
    </select>
  </td>
  <td valign='top'> 
    <h2>Nom du Job</h2>
    <select name='jobname' size='15' class='formulaire' multiple>
<TMPL_LOOP db_jobnames>
      <option><TMPL_VAR jobname></option>
</TMPL_LOOP>
    </select>
  </td>
</tr>
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
<tr>
  <td> <h2> Type </h2> 
 <select name='graph' class='formulaire'>
   <option id='job_size'     value='job_size' title="Taille des jobs pour la p�riode">Taille des job</option>
   <option id='job_duration' value='job_duration' title="Dur�e des jobs pour la p�riode">Dur�e</option>
   <option id='job_rate' value='job_rate' title="D�bit des job pour la p�riode">D�bit des Jobs</option>
   <option id='job_file' value='job_file' title="Nombre de fichier sauvegard� par job pour la p�riode">Job Files</option>
   <option id='job_count_phour' value='job_count_phour' title="Number of job per hour for the period">Job par heure</option>
   <option id='job_count_pday' value='job_count_pday' title="Number of job per day for the period">Job per day</option>
   <option id='job_avg_phour' value='job_avg_pday' title="Average backup size per day for the period">Job avg B/hour</option>
   <option id='job_avg_pday' value='job_avg_pday' title="Average backup size per hour for the period">Job avg B/day</option>
   <option id='job_sum_phour' value='job_sum_phour' title="Job size per hour">Job total B/hour</option>
   <option id='job_sum_pday' value='job_sum_pday' title="Job size per day">Job total B/day</option>
   <option id='job_count_hour' value='job_count_hour' title="Number of job per hour for the period">Jobs Count (h)</option>
   <option id='job_count_day' value='job_count_day' title="Number of job per day for the period">Jobs Count (d)</option>
   <option id='job_avg_hour' value='job_avg_hour' title="Average backup size per hour for the period">Job avg size (h)</option>
   <option id='job_avg_day' value='job_avg_day' title="Average backup size per day for the period">Job avg size (d)</option>
   <option id='job_sum_hour' value='job_sum_hour' title="Job size per hour for the period">Job Bytes (h)</option>
   <option id='job_sum_day' value='job_sum_day' title="Job size per day for the period">Job Bytes (d)</option>
   <option onclick='document.getElementById("gtype_balloon").selected=true;' id='job_time_nb' value='job_time_nb' title="Affiche la dur�e, la taille et le nombre de fichier avec des cercles">Time,size,files</option>
   <option onclick='document.getElementById("gtype_balloon").selected=true;' id='job_time_size' value='job_time_size' title="Affiche la dur�e, le nombre de fichier et la taille avec des cercles">Time,files,size</option>

 </select>
  </td>
  <td valign='bottom'> 
    <h2>Nombre d'&eacute;l&eacute;ments</h2>
    <input type='text' name='limit' value='<TMPL_VAR limit>' 
        class='formulaire' size='4'>
  </td>
</tr>
<tr>
<td><h2> Type de graphique </h2> 
  <select name='gtype' class='formulaire'>
    <option id='gtype_bars' value='bars'>Barres</option>
<!--  <option id='gtype_bars3d' value='bars3d'>Bars3d</option> -->
    <option id='gtype_lines' value='lines'>Lignes</option>
    <option onclick='document.getElementById("job_time_size").selected=true;' id='gtype_balloon' value='balloon'>Balloon</option>
    <option id='gtype_linespoints' value='linespoints'>Lignes avec points</option>
</td>
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

 <img usemap='imggraph' id='imggraph' 
      alt='Rien n a afficher, essayer avec une autre p�riode'>
 </div>

</td>
</table>
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

  <TMPL_IF graph>
     document.getElementById('<TMPL_VAR graph>').selected=true;
  </TMPL_IF>

  <TMPL_IF gtype>
     document.getElementById('gtype_<TMPL_VAR gtype>').selected=true;
  </TMPL_IF>

  <TMPL_IF url>
   document.getElementById('imggraph').src='bgraph.pl?<TMPL_VAR url>'
  </TMPL_IF>

  <TMPL_LOOP qclient_groups>
     document.getElementById('group_' + <TMPL_VAR name>).selected = true;
  </TMPL_LOOP>
</script>
