<br/>
<div class='titlediv'>
  <h1 class='newstitle'> 
   D�placer des m�dias
  </h1>
 </div>
 <div class='bodydiv'>

<form action="?" method='GET'>
<table>
<tr>
<td><b>To: </b></td><td><input class='formulaire' name='email' value='<TMPL_VAR email>'></td>
</tr><tr>
<td><b>Subject: </b></td><td><input class='formulaire' name='subject' value='[BACULA] D�placer des m�dias vers <TMPL_VAR newlocation>' size='80'></td>
</tr><tr>
<td></td>
<td>
<textarea name='content' class='formulaire' cols='80' rows='32'>
Bonjour,

Pouvez vous d�placer ces m�dias vers <TMPL_VAR newlocation>
Media :
<TMPL_LOOP media>
 - <TMPL_VAR VolumeName>  (<TMPL_VAR location>)
</TMPL_LOOP>

Quand cela sera termin�, pouvez vous cliquer sur le lien ci-dessous pour mettre � jour la localisation ?
(Vous pouvez utiliser ce lien : <TMPL_VAR url>).

Merci
</textarea>
</td></tr></table>
<input class='formulaire' type='submit' name='action' value='move_email'>
</form>
<br>
<a href="<TMPL_VAR url>"><img alt='Mettre � jour maintenant' src='/bweb/update.png'>Mettre � jour maintenant</a>
</div>
