import {withKingdomContext} from "@derivean/kingdom";
import {type IContainer}    from "@use-pico/container";

const context = [
    withKingdomContext,
] as const;

export const withContext: IContainer.Register = container => {
    context.forEach(register => register(container));
};
