class pointCor{
  private int x;
  private int y;
  pointCor(int x1, int y1){
    x = x1;
    y = y1;
  }
  public  void setX(int k){
    x = k;
  }
  public  void setY(int k){
    y = k;
  }
  public int getX(){
    return x;
  }
  public int getY(){
    return y;
  }
}

class rectExp{
  private int x, y, len, hei;
  rectExp(int x1,int y1,int len1,int hei1){
    x = x1;
    y = y1;
    len = len1;
    hei = hei1;
  }
  public void rectDraw(){
    rect(x,y,len,hei);
  }
  public int x(){
    return x;
  }
  public int y(){
    return y;
  }
  public int len(){
    return len;
  }
  public int hei(){
    return hei;
  }
  public void setX(int k){
    x = k;
  }
  public void setY(int k){
    y = k;
  }
  public void setLen(int k){
    len = k;
  }
  public void setHei(int k){
    hei = k;
  }
}

void arrow(int x,int y){
  line(x,y,x,y-20);
  line(x,y,x-7,y-7);
  line(x,y,x+7,y-7);
}

void arrowX(int x,int y){
  line(x,y,x+20,y);
  line(x+20,y,x+13,y-7);
  line(x+20,y,x+13,y+7);
}

void drawAreaBlock(int x,rectExp exp,int blockNum){
  
  int step = exp.len();
  int x1 = exp.x();
  for(int i = 0;i < blockNum;i++){
    //stroke(255,0,0);
    //rect(x+15+step*i,y+5,step,steph);
    exp.setX(x1+step*i);
    exp.rectDraw();
  }
  exp.setX(x+15);
}

int verifyArea(rectExp exp,pointCor point,int len){
  int x = point.getX(),y = point.getY(),step = exp.len();
  int preserve = exp.x();
  if(x < len){
    arrow(x,y);
    exp.setX(x);
    fill(255,0,0);
    exp.rectDraw();
    x += step;
    point.setX(x);
  }
  exp.setX(preserve);
  return x;
}

int searchBase(pointCor point,int hei){
  int x = point.getX(), y = point.getY();
  if(y < hei){
    arrow(x,y);
    y += 20;
    point.setY(y);
  }
  return y;
}

int getPage(rectExp exp,pointCor point,int len){
  int x  = point.getX(),y = point.getY(),step = exp.len();
  arrow(x,y);
  fill(255,0,0);
  while(x <= len){
    exp.setX(x);
    exp.rectDraw();
    x += step;
  }
  return x;
}

int structCopy(rectExp exp,int ter){
  int x = exp.x(),y = exp.y(),step = exp.len();
  noFill();
  while(x <= ter){
    exp.setX(x);
    exp.rectDraw();
    x += step;
  }
  return x;
}

int searchStruct(pointCor point,int len){
  int x = point.getX(), y = point.getY();
  if(x < len){
    arrowX(x,y);
    x += 50;
    point.setX(x);
  }
  return x;
}

rectExp exp = new rectExp(65,105,50,50);
pointCor point = new pointCor(65,105);
rectExp exp1 = new rectExp(65,175,50,50);
pointCor point1 = new pointCor(65,175);
pointCor point2[] = new pointCor[8];
PImage photo[] = new PImage[8];



int lenx = 65,heiy = 105,lenx1 = 65;
int lenx2[] = {155,155,155,155,155,155,155,155};
void setup(){
  size(1000,1000);
  frameRate(5);
  for(int i = 0;i < 8;i++){
    point2[i] = new pointCor(155,i*55+150);
  }
  photo[0] = loadImage("../uninterruptable.jpeg");
}
void draw(){
  background(255);
  int len = 850, hei = 150;
  noFill();
  rect(50,100,len,hei);
  drawAreaBlock(50,exp,16);
  if(lenx < len){
    lenx = verifyArea(exp,point,len);
    return;
  }
  
  point.setX(65);
  noFill();
  drawAreaBlock(50,exp1,1);
  
  if(heiy < 175){
    heiy = searchBase(point, 180);
    fill(125,125,125);
    exp.setX(65);
    exp.rectDraw();
    return;
  }
  
  if(lenx1 < len){
    lenx1 = getPage(exp1,point1,lenx1);
    return;
  }
  getPage(exp1,point1,lenx1-50);
  
  clear();
  background(255);
  noFill();
  rect(100,100,200,600);
  rect(400,100,200,600);
  for(int i = 0;i < 8;i++){
    rect(155,125+i*55,50,50);
    rect(455,125+i*55,50,50);
  }
  for(int i = 0;i < 8;i++){ //<>//
    if(lenx2[i] <= 465){
      lenx2[i] = searchStruct(point2[i],465);
      ellipse(650,400,50,40);
      ellipse(710,330,70,60);
      ellipse(800,230,170,140);
      image(photo[0],750,180);
      return;
    }
  }
  //for(int i = 0;i < 8;i++){
    //structCopy(struct[i],455);
  //}
}