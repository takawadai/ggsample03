//
// ゲームグラフィックス特論宿題アプリケーション
//
#include "GgApp.h"

// プロジェクト名
#ifndef PROJECT_NAME
#  define PROJECT_NAME "ggsample03"
#endif

// オブジェクト関連の処理
#include "object.h"

// シェーダー関連の処理
#include "shader.h"

//
// 4 行 4 列の行列の積を求める
//
//   m ← m1 × m2
//
static void multiply(GLfloat* m, const GLfloat* m1, const GLfloat* m2)
{
  for (int i = 0; i < 16; ++i)
  {
    int j = i & 3, k = i & ~3;

    // 配列変数に行列が転置された状態で格納されていることを考慮している
    m[i] = m1[0 + j] * m2[k + 0] + m1[4 + j] * m2[k + 1] + m1[8 + j] * m2[k + 2] + m1[12 + j] * m2[k + 3];
  }
}

//
// 単位行列を設定する
//
//   m: 単位行列を格納する配列
//
static void loadIdentity(GLfloat* m)
{
  m[ 0] = m[ 5] = m[10] = m[15] = 1.0f;
  m[ 1] = m[ 2] = m[ 3] = m[ 4] =
  m[ 6] = m[ 7] = m[ 8] = m[ 9] =
  m[11] = m[12] = m[13] = m[14] = 0.0f;
}

//
// 直交投影変換行列を求める
//
//   m: 直交投影変換行列を格納する配列
//   left, right: ビューボリュームの左右端
//   bottom, top: ビューボリュームの上下端
//   zNear, zFar: 前方面および後方面までの距離
//
static void ortho(GLfloat* m, float left, float right, float bottom, float top, float zNear, float zFar)
{
  m[ 0] = 2.0f / (right - left);
  m[ 5] = 2.0f / (top - bottom);
  m[10] = -2.0f / (zFar - zNear);
  m[12] = -(right + left) / (right - left);
  m[13] = -(top + bottom) / (top - bottom);
  m[14] = -(zFar + zNear) / (zFar - zNear);
  m[15] = 1.0f;
  m[ 1] = m[ 2] = m[ 3] = m[ 4] = m[ 6] = m[ 7] = m[ 8] = m[ 9] = m[11] = 0.0f;
}

//
// 透視投影変換行列を求める
//
//   m: 透視投影変換行列を格納する配列
//   left, right: 前方面の左右端
//   bottom, top: 前方面の上下端
//   zNear, zFar: 前方面および後方面までの距離
//
static void frustum(GLfloat* m, float left, float right, float bottom, float top, float zNear, float zFar)
{
  m[ 0] = 2.0f * zNear / (right - left);
  m[ 5] = 2.0f * zNear / (top - bottom);
  m[ 8] = (right + left) / (right - left);
  m[ 9] = (top + bottom) / (top - bottom);
  m[10] = -(zFar + zNear) / (zFar - zNear);
  m[11] = -1.0f;
  m[14] = -2.0f * zFar * zNear / (zFar - zNear);
  m[ 1] = m[ 2] = m[ 3] = m[ 4] = m[ 6] = m[ 7] = m[12] = m[13] = m[15] = 0.0f;
}

//
// 画角を指定して透視投影変換行列を求める
//
//   m: 透視投影変換行列を格納する配列
//   fovy: 画角（ラジアン）
//   aspect: ウィンドウの縦横比
//   zNear, zFar: 前方面および後方面までの距離
//
static void perspective(GLfloat* m, float fovy, float aspect, float zNear, float zFar)
{
  // 【宿題】ここを解答してください（loadIdentity() を置き換えてください）
  //参照：ゲームグラフィックス特論　第3回　講義ノート　p.133-134
  const GLfloat dz(zFar - zNear);
  if (dz != 0.0f)
  {
    m[5] = 1.0f / tan(fovy * 0.5f);
    m[0] = m[5] / aspect;
    m[10] = -(zFar + zNear) / dz;
    m[11] = -1.0f;
    m[14] = -2.0f * zFar * zNear / dz;

    m[1] = m[2] = m[3] = m[4] =
    m[6] = m[7] = m[8] = m[9] =
    m[12] = m[13] = m[15] = 0;
  }
}

static void loadTranslate(float x, float y, float z, GLfloat* m)
{
  //参照：ゲームグラフィックス特論　第3回　講義ノート　p.107-108
  m[12] = x;
  m[13] = y;
  m[14] = z;

  m[1] = m[2] = m[3] =
  m[4] = m[6] = m[7] =
  m[8] = m[9] = m[11] = 0.0f;

  m[0] = m[5] = m[10] = m[15] = 1.0f;
  return;
  //return m;
}

//
// ビュー変換行列を求める
//
//   m: ビュー変換行列を格納する配列
//   ex, ey, ez: 視点の位置
//   gx, gy, gz: 目標点の位置
//   ux, uy, uz: 上方向のベクトル
//
static void lookat(GLfloat* m, float ex, float ey, float ez, float gx, float gy, float gz, float ux, float uy, float uz)
{
  // 【宿題】ここを解答してください（loadIdentity() を置き換えてください）
  //参照：ゲームグラフィックス特論　第3回　講義ノート　p.123-124
  GLfloat tv[16];
  loadTranslate(-ex, -ey, -ez, tv);

  //t軸 = e - g
  const GLfloat tx(ex - gx);
  const GLfloat ty(ey - gy);
  const GLfloat tz(ez - gz);

  //r軸 = u * t軸
  const GLfloat rx(uy * tz - uz * ty);
  const GLfloat ry(uz * tx - ux * tz);
  const GLfloat rz(ux * ty - uy * tx);

  //s軸 = t軸 * r軸
  const GLfloat sx(ty * rz - tz * ry);
  const GLfloat sy(tz * rx - tx * rz);
  const GLfloat sz(tx * ry - ty * rx);

  //s軸の長さチェック
  const GLfloat s2(sx * sx + sy * sy + sz * sz);
  if (s2 == 0.0f) return;

  //回転の変換行列
  GLfloat rv[16];

  //r軸を正規化して配列変数に格納
  const GLfloat r(sqrt(rx * rx + ry * ry + rz * rz));
  rv[0] = rx / r;
  rv[4] = ry / r;
  rv[8] = rz / r;

  //s軸を正規化して配列変数に格納
  const GLfloat s(sqrt(s2));
  rv[1] = sx / s;
  rv[5] = sy / s;
  rv[9] = sz / s;

  //t軸を正規化して配列変数に格納
  const GLfloat t(sqrt(tx * tx + ty * ty + tz * tz));
  rv[2] = tx / t;
  rv[6] = ty / t;
  rv[10] = tz / t;

  // 残りの成分
  rv[3] = rv[7] = rv[11] = rv[12] = rv[13] = rv[14] = 0.0f;
  rv[15] = 1.0f;


  //視点の平行移動の変換行列に視線の変換行列を乗じる
  multiply(m, rv, tv);
  return;

}

//
// アプリケーション本体
//
int GgApp::main(int argc, const char* const* argv)
{
  // ウィンドウを作成する (この行は変更しないでください)
  Window window{ argc > 1 ? argv[1] : PROJECT_NAME };

  // 背景色を指定する
  glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

  // プログラムオブジェクトの作成
  const auto program{ loadProgram(PROJECT_NAME ".vert", "pv", PROJECT_NAME ".frag", "fc") };

  // uniform 変数のインデックスの検索（見つからなければ -1）
  const auto mcLoc{ glGetUniformLocation(program, "mc") };

  // ビュー変換行列を mv に求める
  GLfloat mv[16];
  lookat(mv, 3.0f, 4.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

  // 頂点属性
  static const GLfloat position[][3]
  {
    { -0.9f, -0.9f, -0.9f },  // (0)
    {  0.9f, -0.9f, -0.9f },  // (1)
    {  0.9f, -0.9f,  0.9f },  // (2)
    { -0.9f, -0.9f,  0.9f },  // (3)
    { -0.9f,  0.9f, -0.9f },  // (4)
    {  0.9f,  0.9f, -0.9f },  // (5)
    {  0.9f,  0.9f,  0.9f },  // (6)
    { -0.9f,  0.9f,  0.9f },  // (7)
  };

  // 頂点数
  constexpr auto vertices{ static_cast<GLuint>(std::size(position)) };

  // 頂点インデックス
  static const GLuint index[]
  {
    0, 1,
    1, 2,
    2, 3,
    3, 0,
    0, 4,
    1, 5,
    2, 6,
    3, 7,
    4, 5,
    5, 6,
    6, 7,
    7, 4,
  };

  // 稜線数
  constexpr auto lines{ static_cast<GLuint>(std::size(index)) };

  // 頂点配列オブジェクトの作成
  const auto vao{ createObject(vertices, position, lines, index) };

  // ウィンドウが開いている間繰り返す
  while (window)
  {
    // ウィンドウを消去する
    glClear(GL_COLOR_BUFFER_BIT);

    // シェーダプログラムの使用開始
    glUseProgram(program);

    // 投影変換行列 mp を求める (window.getAspect() はウィンドウの縦横比)
    GLfloat mp[16];
    perspective(mp, 0.5f, window.getAspect(), 1.0f, 15.0f);

    // 投影変換行列 mp とビュー変換行列 mv の積を変換行列 mc に求める
    GLfloat mc[16];
    multiply(mc, mp, mv);

    // uniform 変数 mc に変換行列 mc を設定する
    // 【宿題】ここを解答してください（uniform 変数 mc のインデックスは変数 mcLoc に入っています）

    //参照　「床井研究室 - 第５回 座標変換」https://marina.sys.wakayama-u.ac.jp/~tokoi/?date=20090829
    glUniformMatrix4fv(mcLoc, 1, GL_FALSE, mc);
    
    //glUniform1fv(mcLoc, 16, mc);
   
    // 描画に使う頂点配列オブジェクトの指定
    glBindVertexArray(vao);

    // 図形の描画
    glDrawElements(GL_LINES, lines, GL_UNSIGNED_INT, 0);

    // 頂点配列オブジェクトの指定解除
    glBindVertexArray(0);

    // シェーダプログラムの使用終了
    glUseProgram(0);

    // カラーバッファを入れ替えてイベントを取り出す
    window.swapBuffers();
  }

  return 0;
}
