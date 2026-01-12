
![11](https://github.com/user-attachments/assets/c64cb4e3-382e-4baa-818e-c55dfa2720d0)

# Planar Clipping Component (Unreal Engine)

이 프로젝트는 Unreal Engine 환경에서 특정 평면($FPlane$)을 기준으로 액터의 **시각적 메시 클리핑**과 **물리 콜리전(Physics Collision) 상태**를 실시간으로 동기화하여 제어하는 `UActorComponent`입니다.

단순히 보이지 않게 처리하는 것을 넘어, 잘려 나간 부위의 물리 판정까지 정밀하게 제거하는 것이 핵심입니다.

---

## 핵심 기능 (Key Features)

* **Dynamic Mesh Clipping**: 머티리얼 파라미터($Ax + By + Cz + D = 0$)를 통해 평면 뒷부분의 메시를 실시간으로 렌더링에서 제외합니다.
* **Physics-Synced Clipping**: 메시가 잘려나가는 지점에 맞춰 스켈레탈 메시의 각 물리 바디(Sphere, Box, Sphyl) 충돌을 자동으로 On/Off 합니다.
* **High Performance**: 매 프레임 모든 계산을 수행하는 대신, 평면 활성화/비활성화 시점에 각 Shape의 World Transform을 계산하여 효율적으로 물리 상태를 갱신합니다.

---

## 주요 함수 및 로직 설명

### 1️⃣ 시각적 클리핑 제어
* **`ActivateHiddenPlaneFromActor(AActor* PlaneActor)`**
    * 입력받은 액터의 위치와 `UpVector`를 추출하여 정규화된 `FPlane` 데이터를 생성합니다.
* **`ApplyPlaneToMaterials()`**
    * 액터에 포함된 모든 `UMeshComponent`를 탐색합니다.
    * `MaterialInstanceDynamic(MID)`을 생성하여 셰이더 내의 `ClippingPlane` 파라미터에 평면 데이터를 실시간 전달합니다.

### 2️⃣ 물리 콜리전 동기화 로직
**`UpdateSkeletalShapeCollision()`** 함수는 Physics Asset의 각 도형을 순회하며 평면과의 위치 관계를 판정합니다.

| 판정 함수 | 알고리즘 설명 |
| :--- | :--- |
| **`IsSphereAbovePlane`** | 평면의 방정식($PlaneDot$)을 통해 구체의 중심점과 평면 사이의 거리가 반지름($R$)보다 큰지 판별합니다. |
| **`IsBoxAbovePlane`** | **OBB(Oriented Bounding Box)** 판정. 박스의 각 축을 평면 법선에 투영하여 투영 반지름($r$)을 계산하고, 중심점의 위치와 비교합니다. |
| **`IsCapsuleAbovePlane`** | 캡슐을 구성하는 선분(Segment)의 양 끝점과 평면 사이의 거리 중 최솟값이 반지름보다 큰지 판별합니다. |

---

## 수학적 구현 (Math)

박스(Box) 충돌체가 평면 위에 완전히 있는지 판별하기 위해 분리축 이론(SAT)의 개념을 응용하여 구현했습니다.

$$r = |N \cdot A_0| \cdot e_x + |N \cdot A_1| \cdot e_y + |N \cdot A_2| \cdot e_z$$

```cpp
// OBB(Oriented Bounding Box) 판정 로직 예시
const float r =
    FMath::Abs(FVector::DotProduct(N, A0)) * HalfExtent.X +
    FMath::Abs(FVector::DotProduct(N, A1)) * HalfExtent.Y +
    FMath::Abs(FVector::DotProduct(N, A2)) * HalfExtent.Z;

// 평면으로부터의 거리가 투영 반지름보다 크면 '위에 있음'으로 판정
return Plane.PlaneDot(C) > r;
```
<img width="1157" height="510" alt="image" src="https://github.com/user-attachments/assets/3f285216-80dc-440b-acb7-9c19d3d4be7a" />

