import Image from "next/image";

export default function Spinner() {
  return (
    <div className="loading-container">
      <div className="spinner-container">
        <Image
          src={"/assets/chip.svg"}
          alt="Loading..."
          width={100}
          height={100}
          priority
        />
      </div>
    </div>
  );
}
