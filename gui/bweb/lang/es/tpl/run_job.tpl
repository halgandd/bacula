<br/>
 <div class='titlediv'>
  <h1 class='newstitle'> Jobs Definidos: </h1>
 </div>
 <div class='bodydiv'>
  <form name='form1' action='?' method='GET'>  
  <table border='0'>

   <tr><td>Nombre Job: </td><td>
   <select name='job'>
    <TMPL_LOOP jobs>
     <option value='<TMPL_VAR name>'>
       <TMPL_VAR name>
     </option>
    </TMPL_LOOP>
   </select>
   </td></tr>
   </table>
   <br/>
   <button type="submit" class="bp" name='action' value='enable_job' title='Activar'> <img src='/bweb/inflag1.png' alt=''> Activar </button>
   <button type="submit" class="bp" name='action' value='disable_job' title='Desactivar' > <img src='/bweb/inflag0.png' alt=''> Desactivar </button>
   <button type="submit" class="bp" name='action' value='next_job2' title='Show schedule' > <img src='/bweb/zoom.png' alt=''> Show schedule  </button>
   <button type="submit" class="bp" name='action' value='run_job_mod' title='Ejecutar Ahora' > <img src='/bweb/R.png' alt=''> Ejecutar Ahora  </button>
  </form>
 </div>
