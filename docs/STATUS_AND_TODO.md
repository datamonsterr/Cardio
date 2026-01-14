# Trạng thái dự án và TODO List

## Đã hoàn thành (Checked)

### Server Side
- [x] Authentication: Login/Signup hoàn chỉnh
- [x] Table Management: Create table, Get tables
- [x] Join Table: Handler đã có, gửi game state khi join
- [x] Action Request: Handler đã có, xử lý fold/check/call/bet/raise
- [x] Broadcast Game State: Sau mỗi action, broadcast UPDATE_GAMESTATE (600) tới tất cả players
- [x] Scoreboard: dbGetScoreBoard, handle_get_scoreboard
- [x] Friend List: dbGetFriendList, handle_get_friendlist

### Client Side
- [x] Login/Signup: Hoàn chỉnh
- [x] Tables Page: Hiển thị danh sách bàn, filter, search
- [x] Create Table: Button và modal đã có, kết nối với server

## Cần làm (TODO - theo thứ tự ưu tiên)

### Server Side

#### 1. Leave Table (Ưu tiên cao)
- [ ] `handle_leave_table_request` trong handler.c - hiện tại hàm rỗng
- [ ] Cần gọi `leave_table()` và broadcast update state
- [ ] Lưu hand history trước khi leave (nếu đang trong game)

#### 2. Friend Management (Ưu tiên trung bình)
- [ ] `dbAddFriend` trong lib/db/src/user.c
- [ ] `dbRemoveFriend` trong lib/db/src/user.c
- [ ] `handle_add_friend` trong handler.c
- [ ] `handle_remove_friend` trong handler.c
- [ ] `handle_invite_friend` trong handler.c (có thể dùng packet type mới)
- [ ] Đăng ký handlers trong main.c switch case

#### 3. Save Hand History (Ưu tiên trung bình)
- [ ] Tạo bảng HandHistory trong database:
  ```sql
  CREATE TABLE HandHistory (
    hand_id SERIAL PRIMARY KEY,
    table_id INTEGER,
    user_id INTEGER,
    hand_result VARCHAR(20), -- 'win', 'loss', 'fold'
    amount_won_loss INTEGER,
    hand_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES "User"(user_id)
  );
  ```
- [ ] `dbSaveHandHistory` trong lib/db/src/user.c
- [ ] Gọi sau mỗi hand complete hoặc khi player leave table

#### 4. Update Balance sau mỗi hand (Ưu tiên cao)
- [ ] Cập nhật balance trong database sau mỗi hand
- [ ] Gọi `dbUpdateUserBalance` sau khi hand kết thúc

### Client Side

#### 1. Join Table (Ưu tiên cao)
- [ ] Trong TablesPage.tsx, khi click "Join Table", gửi JOIN_TABLE_REQUEST (400)
- [ ] Lắng nghe response, nếu OK thì navigate đến GamePage
- [ ] Nếu nhận được game state trong response, cập nhật GamePage state

#### 2. Leave Table (Ưu tiên cao)
- [ ] Thêm nút "Leave Table" trong GamePage.tsx
- [ ] Gửi LEAVE_TABLE_REQUEST (700) khi click
- [ ] Navigate về TablesPage sau khi leave thành công

#### 3. Gameplay thực tế (Ưu tiên rất cao)
- [ ] Thay thế mock logic trong GamePage.tsx
- [ ] Lắng nghe PACKET_UPDATE_GAMESTATE (600) từ TcpClient
- [ ] Cập nhật gameState khi nhận được packet
- [ ] Xử lý các trường hợp: new hand, betting round, player actions

#### 4. Action Request (Ưu tiên rất cao)
- [ ] Khi user click Call/Bet/Fold, gửi ACTION_REQUEST (450)
- [ ] Lắng nghe ACTION_RESULT (451) để hiển thị kết quả
- [ ] Disable buttons khi không phải lượt của user

#### 5. Friend UI (Ưu tiên thấp)
- [ ] Tạo modal/component cho Add/Remove/Invite friend
- [ ] Kết nối với handlers trên server

#### 6. Scoreboard UI (Ưu tiên thấp)
- [ ] Hiển thị scoreboard từ GET_SCOREBOARD (800) response
- [ ] Có thể tích hợp vào HomePage modal

## Ghi chú kỹ thuật

### Protocol Compliance
- Luôn tuân thủ PROTOCOL.md cho packet types
- Sử dụng MessagePack cho tất cả payloads
- Big-Endian cho header (length và packet_type)

### Testing
- Sử dụng `./server/test/e2e_multiplayer_test.c` để test gameplay
- Check `server.log` nếu có lỗi
- Test với nhiều players cùng lúc

### Database
- Mỗi hand có nhiều players thì lưu nhiều records trong HandHistory
- Group by user_id để query dễ dàng
- Update balance sau mỗi hand, không phải sau mỗi action
