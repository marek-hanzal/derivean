import {
    Center,
    VersionStamp
}                from "@use-pico/client";
import {type FC} from "react";

export namespace Footer {
    export type Props = Center.Props;
}

export const Footer: FC<Footer.Props> = props => {
    return <Center
        {...props}
    >
        <VersionStamp/>
    </Center>;
};
