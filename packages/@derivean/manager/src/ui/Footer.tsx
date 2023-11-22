import {VersionStamp} from "@use-pico/client";
import {
    type FC,
    type HTMLAttributes
}                     from "react";

export namespace Footer {
    export type Props = HTMLAttributes<HTMLDivElement>;
}

export const Footer: FC<Footer.Props> = props => {
    return <div
        className={"flex justify-center items-center"}
        {...props}
    >
        <VersionStamp/>
    </div>;
};
