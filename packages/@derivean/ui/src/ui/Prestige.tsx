import {
    css,
    Rating
}                from "@use-pico/client";
import {cn}      from "@use-pico/utils";
import {type FC} from "react";

const colors: Record<number, css.Color> = {
    1: "green",
    2: "blue",
    3: "yellow",
    4: "orange",
    5: "violet",
    6: "grape",
} as const;

export namespace Prestige {
    export interface Props {
        prestige: number;
    }
}

export const Prestige: FC<Prestige.Props> = (
    {
        prestige,
    }
) => {
    return <Rating
        defaultValue={prestige}
        readOnly
        size={"md"}
        emptySymbol={value => <div className={cn(
            colors[value] ?? "green",
            "i-tabler-star",
        )}/>}
        fullSymbol={value => <div className={cn(
            colors[value] ?? "green",
            "i-tabler-star-filled",
        )}/>}
        count={6}
    />;
};
