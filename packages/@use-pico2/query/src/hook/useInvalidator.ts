import {useQueryClient}    from "@tanstack/react-query";
import {type IInvalidator} from "../api/IInvalidator";

export namespace useInvalidator {
    export interface Props {
        invalidator: IInvalidator;
    }
}

export const useInvalidator = (
    {
        invalidator: {
                         invalidator,
                         key
                     },
    }: useInvalidator.Props
) => {
    const queryClient = useQueryClient();
    return invalidator ? (async () => {
        return invalidator({
            queryClient,
        });
    }) : (async () => {
        return queryClient.invalidateQueries({
            queryKey: key,
        });
    });
};
