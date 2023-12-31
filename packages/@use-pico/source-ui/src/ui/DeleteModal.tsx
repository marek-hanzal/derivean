"use client";

import {Translation}         from "@use-pico/i18n";
import {
    type IWithMutation,
    useMutation
}                            from "@use-pico/query";
import {type ResponseSchema} from "@use-pico/schema";
import {type MutationSchema} from "@use-pico/source";
import {useStore}            from "@use-pico/store";
import {type WithEntity}     from "@use-pico/types";
import {
    Button,
    CloseIcon,
    Divider,
    Flex,
    Modal,
    ModalStore,
    TrashIcon,
    useSuccessNotification
}                            from "@use-pico/ui";

export namespace DeleteModal {
    export interface Props<
        TMutationSchema extends MutationSchema<any, any>,
        TResponseSchema extends ResponseSchema,
    > extends Modal.Props, WithEntity.Schema<TResponseSchema> {
        withMutation: IWithMutation<
            TMutationSchema,
            TResponseSchema
        >;
    }
}

export const DeleteModal = <
    TMutationSchema extends MutationSchema<any, any>,
    TResponseSchema extends ResponseSchema,
>(
    {
        withMutation,
        entity,
        ...props
    }: DeleteModal.Props<TMutationSchema, TResponseSchema>
) => {
    const {close} = useStore(ModalStore, ({close}) => ({close}));
    const successNotification = useSuccessNotification();
    const deleteMutation = useMutation({withMutation});

    return <Modal
        modalProps={{
            closeOnClickOutside: !deleteMutation.isPending,
        }}
        title={"delete.title"}
        {...props}
    >
        <Translation withLabel={"delete.content"} values={entity}/>
        <Divider mt={"sm"} mb={"sm"}/>
        <Flex
            justify={"space-between"}
            align={"center"}
        >
            <Button
                leftSection={<CloseIcon/>}
                size={"md"}
                color={"blue"}
                variant={"subtle"}
                disabled={deleteMutation.isPending || deleteMutation.isSuccess}
                onClick={() => close(props.modalId)}
            >
                <Translation namespace={"common"} withLabel={"cancel.button"}/>
            </Button>
            <Button
                size={"lg"}
                color={"red"}
                leftSection={<TrashIcon/>}
                loading={deleteMutation.isPending || deleteMutation.isSuccess}
                onClick={() => {
                    deleteMutation.mutate({
                        delete: {
                            where: {
                                id: entity.id,
                            },
                        },
                    }, {
                        onSuccess: response => {
                            successNotification({
                                withTranslation: {
                                    label:  "delete",
                                    values: response,
                                },
                            });
                        },
                        onSettled: () => close(props.modalId),
                    });
                }}
            >
                <Translation withLabel={"delete.confirm.button"}/>
            </Button>
        </Flex>
    </Modal>;
};
