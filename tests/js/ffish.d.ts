export function ffishFactory(): Promise<FairyStockfish>;
export default ffishFactory;

export interface FairyStockfish {
    Board: Board;
    Game: Game;
    Notation: typeof Notation;
    Termination: typeof Termination;
    info(): string;
    setOption<T>(name: string, value: T): void;
    setOptionInt(name: string, value: number): void;
    setOptionBool(name: string, value: boolean): void;
    readGamePGN(pgn: string): Game;
    variants(): string;
    loadVariantConfig(variantInitContent: string): void;
    capturesToHand(uciVariant: string): boolean;
    startingFen(uciVariant: string): string;
    validateFen(fen: string, uciVariant?: string, chess960?: boolean): number;
}

export interface Board {
    new(uciVariant?: string, fen?: string, is960?: boolean): Board;
    delete(): void;
    legalMoves(): string;
    legalMovesSan(): string;
    numberLegalMoves(): number;
    push(uciMove: string): boolean;
    pushSan(sanMove: string, notation?: Notation): boolean;
    pop(): void;
    reset(): void;
    is960(): boolean;
    fen(showPromoted?: boolean, countStarted?: number): string;
    setFen(fen: string): void;
    sanMove(uciMove: string, notation?: Notation): string;
    variationSan(uciMoves: string, notation?: Notation, moveNumbers?: boolean): string;
    turn(): boolean;
    fullmoveNumber(): number;
    halfmoveClock(): number;
    gamePly(): number;
    hasInsufficientMaterial(turn: boolean): boolean;
    isInsufficientMaterial(): boolean;
    isGameOver(claimDraw: boolean): boolean;
    result(claimDraw: boolean): string;
    isCheck(): boolean;
    isBikjang(): boolean;
    moveStack(): string;
    pushMoves(uciMoves: string): void;
    pushSanMoves(sanMoves: string, notation?: Notation): void;
    pocket(color: boolean): string;
    toString(): string;
    toVerboseString(): string;
    variant(): string;
}

export interface Game {
    delete(): void;
    headerKeys(): string;
    headers(item: string): string;
    mainlineMoves(): string;
}

export enum Notation {
    DEFAULT,
    SAN,
    LAN,
    SHOGI_HOSKING,
    SHOGI_HODGES,
    SHOGI_HODGES_NUMBER,
    JANGGI,
    XIANGQI_WXF,
    THAI_SAN,
    THAI_LAN,
}

export enum Termination {
    ONGOING,
    CHECKMATE,
    STALEMATE,
    INSUFFICIENT_MATERIAL,
    N_MOVE_RULE,
    N_FOLD_REPETITION,
    VARIANT_END,
}
