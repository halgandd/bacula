<br/>
 <div class='titlediv'>
  <h1 class='newstitle'>Locations</h1>
 </div>
 <div class="bodydiv">
   <form action='?' method='get'>
    <table id='id<TMPL_VAR ID>'></table>
    <button type="submit" class="bp" name='action' value='location_add' title='Add a location'> <img src='/bweb/add.png' alt=''>Add</button>
    <button type="submit" class="bp" name='action' value='location_del' onclick='confirm("Do you want to remove this location?")' title='Remove a location'> <img src='/bweb/remove.png' alt=''>Remove</button>
    <button type="submit" class="bp" name='action' value='location_edit' title='Edit a location'> <img src='/bweb/edit.png' alt=''>Edit</button>

    <button type="submit" class="bp" name='action' value='media' title='View content'> <img src='/bweb/zoom.png' alt=''>View content</button>
   </form>
 </div>

<script type="text/javascript" language="JavaScript">

var header = new Array("Name","Enabled", "Cost", "Nb volumes", "Select");

var data = new Array();
var chkbox;

var img;

<TMPL_LOOP Locations>
img = document.createElement('IMG');
img.src = '/bweb/inflag<TMPL_VAR enabled>.png';

chkbox = document.createElement('INPUT');
chkbox.type  = 'radio';
chkbox.name  = 'location';
chkbox.value = '<TMPL_VAR Location>';

data.push( new Array(
"<TMPL_VAR Location>",
img,
"<TMPL_VAR Cost>",
"<TMPL_VAR name=volnum>",
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
// natural_compare: false,
 even_cell_color: even_cell_color,
 odd_cell_color: odd_cell_color, 
 header_color: header_color,
 page_nav: true,
 padding: 3,
// disable_sorting: new Array(5,6),
 rows_per_page: rows_per_page
}
);
</script>
