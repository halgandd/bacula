<br/>
<div class='titlediv'>
  <h1 class='newstitle'> Assistant d'internalisation de m�dias (partie 1/2)</h1>
</div>
<div class="bodydiv">
Cet outil va s�lectionner pour vous les meilleurs cartouches � internaliser.
Vous devrez choisir parmi la s�lection de l'�cran suivant.

  <form action="?" method='GET'>
   <table>
    <tr><td>Pool :</td>      
        <td><select name='pool' class='formulaire' multiple>
<TMPL_LOOP db_pools>
             <option><TMPL_VAR name></option>
</TMPL_LOOP>
           </select>
        </td>
    </tr>
    <tr><td>Type de m�dia :</td>      
        <td><select name='mediatype' class='formulaire' multiple>
<TMPL_LOOP db_mediatypes>
             <option><TMPL_VAR mediatype></option>
</TMPL_LOOP>
           </select>
        </td>
    </tr>
    <tr><td>
    Localisation : 
    </td><td><select name='location' class='formulaire'>
  <TMPL_LOOP db_locations>
      <option value='<TMPL_VAR location>'><TMPL_VAR location></option>
  </TMPL_LOOP>
    </select>
    </td>
    </tr>
    <tr>
        <td>Choisir des m�dias <br/> expir�s :</td> 
        <td> <input type='checkbox' name='expired' class='formulaire' 
		checked> </td>
    </tr>
    <tr>
        <td>Nombre de m�dias <br/> � internaliser :</td> 
        <td> <input type='text' name='limit' class='formulaire' 
		size='3' value='10'> </td>
    </tr>
    <tr>
        <td><input type="image" name='action' value='compute_intern_media' 
		title='Suivant' src='/bweb/next.png'>
        </td><td/>
    </tr>
   </table>
   </form>
</div>
